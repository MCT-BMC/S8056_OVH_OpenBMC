#!/usr/bin/env python

# TODO: openbmc/openbmc#2994 remove python 2 support
try:  # python 2
    import gobject
except ImportError:  # python 3
    from gi.repository import GObject as gobject
import dbus
import dbus.service
import dbus.mainloop.glib
import subprocess
import tempfile
import shutil
import tarfile
import os, sys
from obmc.dbuslib.bindings import get_dbus, DbusProperties, DbusObjectManager

DBUS_NAME = 'org.openbmc.control.BiosFlash'
OBJ_NAME = '/org/openbmc/control/flash/bios'
DOWNLOAD_INTF = 'org.openbmc.managers.Download'

HOST_DBUS_NAME = 'xyz.openbmc_project.State.Host'
HOST_OBJ_NAME =  '/xyz/openbmc_project/state/host0'

UPDATE_PATH = '/run/initramfs'

def doExtract(members, files):
    for tarinfo in members:
        if tarinfo.name in files:
            yield tarinfo

def save_fw_env():
    fw_env = "/etc/fw_env.config"
    lines = 0
    files = []
    envcfg = open(fw_env, 'r')
    try:
        for line in envcfg.readlines():
            # ignore lines that are blank or start with #
            if (line.startswith("#")):
                continue
            if (not len(line.strip())):
                continue
            fn = line.partition("\t")[0]
            files.append(fn)
            lines += 1
    finally:
        envcfg.close()
    if (lines < 1 or lines > 2 or (lines == 2 and files[0] != files[1])):
            raise Exception("Error parsing %s\n" % fw_env)
    shutil.copyfile(files[0], os.path.join(UPDATE_PATH, "image-u-boot-env"))

class BiosFlashControl(DbusProperties, DbusObjectManager):
    def __init__(self, bus, name):
        super(BiosFlashControl, self).__init__(
            conn=bus,
            object_path=name)

        self.Set(DBUS_NAME, "status", "Idle")
        self.Set(DBUS_NAME, "filename", "")
        self.Set(DBUS_NAME, "prepare_update", False)

        bus.add_signal_receiver(
            self.download_error_handler, signal_name="DownloadError")
        bus.add_signal_receiver(
            self.download_complete_handler, signal_name="DownloadComplete")

        self.update_process = None
        self.progress_name = None

    @dbus.service.method(
        DBUS_NAME, in_signature='ss', out_signature='')
    def updateBIOSViaTftp(self, ip, filename):
        self.Set(DBUS_NAME, "status", "Downloading")
        self.TftpDownload(ip, filename)

    @dbus.service.method(
        DBUS_NAME, in_signature='s', out_signature='')
    def update(self, filename):
        self.Set(DBUS_NAME, "filename", filename)
        self.download_complete_handler(filename, filename)

    @dbus.service.signal(DOWNLOAD_INTF, signature='ss')
    def TftpDownload(self, ip, filename):
        self.Set(DBUS_NAME, "filename", filename)
        pass

    # Signal handler
    def download_error_handler(self, filename):
        if (filename == self.Get(DBUS_NAME, "filename")):
            self.Set(DBUS_NAME, "status", "Download Error")

    def download_complete_handler(self, outfile, filename):
        # do update
        if (filename != self.Get(DBUS_NAME, "filename")):
            return

        print("Download complete. Updating...")

        self.Set(DBUS_NAME, "status", "Download Complete")
        copy_files = {}

        # determine needed files
        copy_files["image-bios-pnor"] = True

        # copy BIOS image under /run/initramfs path
        dst = os.path.join(UPDATE_PATH, filename)
        shutil.copyfile(outfile, dst)
        os.remove(outfile)
        
        p=subprocess.Popen('ls /run/initramfs', shell=True, stdout=subprocess.PIPE)
        data = p.stdout.read()
        num = data.find(filename)
        
        if num >= 0:
            self.Set(DBUS_NAME, "status", "Image ready to apply.")
        else:
            self.Set(DBUS_NAME, "status", "No image-bios-pnor file under /run/initramfs path.")
        
    
    def Cleanup(self):
        if self.progress_name:
            try:
                os.unlink(self.progress_name)
                self.progress_name = None
            except oserror as e:
                if e.errno == EEXIST:
                    pass
                raise
        self.update_process = None

    @dbus.service.method(
        DBUS_NAME, in_signature='', out_signature='s')
    def GetBIOSUpdateProgress(self):
        msg = ""

        # check Host state
        o = bus.get_object(HOST_DBUS_NAME, HOST_OBJ_NAME)
        intf = dbus.Interface(o, "org.freedesktop.DBus.Properties")
        state = intf.Get(HOST_DBUS_NAME,"CurrentHostState")
        data = state.rsplit('.',1)
            
        if self.Get(DBUS_NAME, "prepare_update"):
            if data[1] == 'Off':
                self.Set(DBUS_NAME, "prepare_update", False)
                self.Set(DBUS_NAME, "status", "System status is Off , Switch SPI interface")
        else:
            if data[1] == 'Running':
                self.Set(DBUS_NAME, "status", "Idle")
                self.Set(DBUS_NAME, "filename", "")
        
        if self.update_process and self.update_process.returncode is None:
            self.update_process.poll()

        if (self.update_process is None):
            pass
        elif (self.update_process.returncode > 0):
            self.Set(DBUS_NAME, "status", "Apply failed")
        elif (self.update_process.returncode is None):
            pass
        else:
            files = ""
            for file in os.listdir(UPDATE_PATH):
                if file.startswith('image-'):
                    files = files + file
            if files == "":
                msg = "Apply Complete.  Reboot to take effect."
            else:
                msg = "Apply Incomplete, Remaining:" + files
            self.Set(DBUS_NAME, "status", msg)

        msg = self.Get(DBUS_NAME, "status") + "\n"
        if self.progress_name:
            try:
                prog = open(self.progress_name, 'r')
                for line in prog:
                    cr = line.rfind("\r", 0, -2)
                    msg = msg + line[cr + 1:]
            except OSError as e:
                if (e.error == EEXIST):
                    pass
                raise
        return msg

    @dbus.service.method(
        DBUS_NAME, in_signature='', out_signature='')
    def Apply(self):
        progress = None
        self.Set(DBUS_NAME, "status", "Writing images to flash")
        try:
            print("Receive method of Apply")
            progress = tempfile.NamedTemporaryFile(
                delete=False, prefix="progress.")
            self.progress_name = progress.name
            self.update_process = subprocess.Popen([
                "/usr/sbin/update_bios.sh"],
                stdout=progress.file,
                stderr=subprocess.STDOUT)
        except Exception as e:
            try:
                progress.close()
                os.unlink(progress.name)
                self.progress_name = None
            except Exception:
                pass
            raise

        try:
            progress.close()
        except Exception:
            pass

    @dbus.service.method(
        DBUS_NAME, in_signature='', out_signature='')
    def PrepareForUpdateBIOS(self):
        print("Prepare For BIOS Update")
        
        o = bus.get_object(HOST_DBUS_NAME, HOST_OBJ_NAME)
        intf = dbus.Interface(o, "org.freedesktop.DBus.Properties")
        state = intf.Get(HOST_DBUS_NAME,"CurrentHostState")
        data = state.rsplit('.',1)
        self.Set(DBUS_NAME, "prepare_update", True)

        if data[1] == 'Running':
            self.Set(DBUS_NAME, "status", "Wating System Power off")
            # turn off system
            intf.Set(HOST_DBUS_NAME,"RequestedHostTransition","xyz.openbmc_project.State.Host.Transition.Off")
        elif data[1] == 'Quiesced':
            self.Set(DBUS_NAME, "status", "System status is Quiesced")
        else:
            self.Set(DBUS_NAME, "status", "System status is Off , Switch SPI interface")
        
    @dbus.service.method(
        DBUS_NAME, in_signature='', out_signature='')
    def SwitchSPIInterface(self):
        self.Set(DBUS_NAME, "status", "Connect SPI interface to BMC")
        
        # into recovery mode, no recovery mode pin on S7106
        #subprocess.Popen('gpioset gpiochip0 72=0', shell=True)
        
        # connect BMC to SPI Flash 
        subprocess.Popen('gpioset gpiochip0 72=0', shell=True)
        subprocess.Popen('gpioset gpiochip0 73=1', shell=True)

        # Load the ASpeed SMC driver
        subprocess.Popen('echo 1e630000.spi > /sys/bus/platform/drivers/aspeed-smc/bind', shell=True)

        self.Set(DBUS_NAME, "status", "Prepare Update BIOS")

    @dbus.service.method(
        DBUS_NAME, in_signature='', out_signature='')
    def RestoreSPIInterface(self):
        # unLoad the ASpeed SMC driver
        subprocess.Popen('echo 1e630000.spi > /sys/bus/platform/drivers/aspeed-smc/unbind', shell=True)
        
        # connect PCH to SPI Flash 
        subprocess.Popen('gpioget gpiochip0 72', shell=True)
        subprocess.Popen('gpioget gpiochip0 73', shell=True)

        #self.update_process = None
        if self.update_process:
            try:
                self.update_process.kill()
            except Exception:
                pass
        
        self.Cleanup()
        self.Set(DBUS_NAME, "status", "Start Power On System")
        
        # power on system
        o = bus.get_object(HOST_DBUS_NAME, HOST_OBJ_NAME)
        intf = dbus.Interface(o, "org.freedesktop.DBus.Properties")
        intf.Set(HOST_DBUS_NAME,"RequestedHostTransition","xyz.openbmc_project.State.Host.Transition.On")

if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    bus = get_dbus()
    obj = BiosFlashControl(bus, OBJ_NAME)
    mainloop = gobject.MainLoop()

    obj.unmask_signals()
    name = dbus.service.BusName(DBUS_NAME, bus)

    print("Running BIOS Flash Control")
    mainloop.run()

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
