#
# Copyright (C) 2020-present MiTAC Computing Technology Corp.
#
# This file is part of mct_fw_updater.
#
# mct_fw_updater cannot be copied and/or distributed without the express
# permission of MiTAC Computing Technology Corp.
#

#!/usr/bin/env python
"""
Usage: mct_fw_update.py 
                                --file <bin-file> 
                                --bmc <bmc-ip> 
                                --device <updating-device (activebmc , backupbmc , activebios , backupbios , cpld , psu1 , psu2)>
                                --generate <enable generate mode>
                                [-v]

This scripts copies firmware image file to BMC,
and uses REST APIs to update the firmware image file to purpose device via bmc.
"""

import argparse
import json
import os
import sys
import requests
import tarfile
from time import sleep
from os import path

utilityVersion = '0.19'

updateTimeout = 600
jsonPrepareUpdate = 0

deviceList = ["activebmc",
              "backupbmc",
              "activebios",
              "backupbios",
              "cpld",
              "psu1",
              "psu2",
              "vrMemABCD",
              "vrMemEFGH",
              "vrCPU",
              "vrSOC",
              "pfr",
              "f7a_p0_vddcr_soc",
              "f7a_p0_vdd11",
              "f7a_p0_vddio",
             ]

additionalInfoBuf=""

def deleteLastLine():
    if sys.platform == "win32":
        # TODO : Clear current line for windows OS
        return
    # cursor up one line
    sys.stdout.write('\x1b[1A')
    # delete last line
    sys.stdout.write('\x1b[2K')


def checkBmcAlive(ignore):
    url = DestUrl+'/xyz/openbmc_project/'

    try:
        output = requests.get(url, verify=False, timeout=15)
    except (requests.exceptions.ConnectionError,
            requests.exceptions.Timeout,
            requests.exceptions.TooManyRedirects,
            requests.exceptions.RequestException,
            requests.exceptions.HTTPError) as e:
        if ignore == 'no' :
            print('Requests exceptions :')
            print (e)
        return False

    if FNULL is None:  # Do not print log when FNULL is devnull
        print (output.content)
        print ('Response: %s ms' %(output.elapsed.microseconds/1000))

    if output.status_code == requests.codes.ok:
        return True
    else:
        return False

def setBmcReboot():
    url = DestUrl+'/xyz/openbmc_project/state/bmc0/attr/RequestedBMCTransition'

    try:
        output = requests.put(url, headers={'Content-Type': 'application/json'},
                              json = {'data':"xyz.openbmc_project.State.BMC.Transition.Reboot"},
                              verify=False, timeout=30)
    except (requests.exceptions.ConnectionError,
            requests.exceptions.Timeout,
            requests.exceptions.TooManyRedirects,
            requests.exceptions.RequestException,
            requests.exceptions.HTTPError) as e:
        print('Requests exceptions :')
        print (e)
        return False

    if FNULL is None:  # Do not print log when FNULL is devnull
        print (output.content)
        print ('Response: %s ms' %(output.elapsed.microseconds/1000))

    if output.status_code != requests.codes.ok:
        print (output.content)
        sys.exit(0)

def activeImage(versionId):
    url = DestUrl+'/xyz/openbmc_project/software/' + versionId + '/attr/RequestedActivation'

    try:
        output = requests.put(url, headers={'Content-Type': 'application/json'},
                              json = {'data':"xyz.openbmc_project.Software.Activation.RequestedActivations.Active"},
                              verify=False, timeout=30)
    except (requests.exceptions.ConnectionError,
            requests.exceptions.Timeout,
            requests.exceptions.TooManyRedirects,
            requests.exceptions.RequestException,
            requests.exceptions.HTTPError) as e:
        print('Requests exceptions :')
        print (e)
        return False

    if FNULL is None:  # Do not print log when FNULL is devnull
        print (output.content)
        print ('Response: %s ms' %(output.elapsed.microseconds/1000))

    if output.status_code != requests.codes.ok:
        print (output.content)
        sys.exit(0)

def deleteImage(versionId):
    url = DestUrl+'/xyz/openbmc_project/software/' + versionId + '/action/Delete'

    try:
        output = requests.post(url, headers={'Content-Type': 'application/json'},
                      json = {'data':[]}, verify=False, timeout=30)
    except (requests.exceptions.ConnectionError,
            requests.exceptions.Timeout,
            requests.exceptions.TooManyRedirects,
            requests.exceptions.RequestException,
            requests.exceptions.HTTPError) as e:
        print('Requests exceptions :')
        print (e)
        return False

    if FNULL is None:  # Do not print log when FNULL is devnull
        print (output.content)
        print ('Response: %s ms' %(output.elapsed.microseconds/1000))

    if output.status_code != requests.codes.ok:
        print (output.json().get("data"))
        print ('Failed to delete upload image. Please remove image by manual.')
        sys.exit(0)

def getUpdateProgress(versionId):
    url = DestUrl+'/xyz/openbmc_project/software/' + versionId + '/attr/Progress'

    try:
        output = requests.get(url, headers={'Content-Type': 'application/json'},
                      json = {'data':[]}, verify=False, timeout=30)
    except (requests.exceptions.ConnectionError,
            requests.exceptions.Timeout,
            requests.exceptions.TooManyRedirects,
            requests.exceptions.RequestException,
            requests.exceptions.HTTPError) as e:
        print('Requests exceptions :')
        print (e)
        return ''

    if FNULL is None:  # Do not print log when FNULL is devnull
        print (output.content)
        print ('Response: %s ms' %(output.elapsed.microseconds/1000))

    if output.json().get("status") == 'error':
        return '-1'

    return output.json().get("data")

def getUpdateProgress(versionId):
    url = DestUrl+'/xyz/openbmc_project/software/' + versionId + '/attr/Progress'

    try:
        output = requests.get(url, headers={'Content-Type': 'application/json'},
                      json = {'data':[]}, verify=False, timeout=30)
    except (requests.exceptions.ConnectionError,
            requests.exceptions.Timeout,
            requests.exceptions.TooManyRedirects,
            requests.exceptions.RequestException,
            requests.exceptions.HTTPError) as e:
        print('Requests exceptions :')
        print (e)
        return ''

    if FNULL is None:  # Do not print log when FNULL is devnull
        print (output.content)
        print ('Response: %s ms' %(output.elapsed.microseconds/1000))

    if output.json().get("status") == 'error':
        return '-1'

    return output.json().get("data")

def getUpdateAdditionalInfo(versionId):
    global additionalInfoBuf

    url = DestUrl+'/xyz/openbmc_project/software/' + versionId + '/attr/AdditionalInfo'

    try:
        output = requests.get(url, headers={'Content-Type': 'application/json'},
                      json = {'data':[]}, verify=False, timeout=30)
    except (requests.exceptions.ConnectionError,
            requests.exceptions.Timeout,
            requests.exceptions.TooManyRedirects,
            requests.exceptions.RequestException,
            requests.exceptions.HTTPError) as e:
        print('Requests exceptions :')
        print (e)
        return ''

    if FNULL is None:  # Do not print log when FNULL is devnull
        print (output.content)
        print ('Response: %s ms' %(output.elapsed.microseconds/1000))

    if output.json().get("status") == 'error':
        return '-1'

    if output.json().get("data") == "" or output.json().get("data") == additionalInfoBuf:
        return '-1'

    additionalInfoBuf = output.json().get("data")

    return output.json().get("data")

def getCurrentBmcBootSource():
    url = DestUrl+'/xyz/openbmc_project/oem/BmcStatus/attr/bootSourceStatus'

    try:
        output = requests.get(url, headers={'Content-Type': 'application/json'},
                      json = {'data':[]}, verify=False, timeout=30)
    except (requests.exceptions.ConnectionError,
            requests.exceptions.Timeout,
            requests.exceptions.TooManyRedirects,
            requests.exceptions.RequestException,
            requests.exceptions.HTTPError) as e:
        print('Requests exceptions :')
        print (e)
        return '-1'

    if FNULL is None:  # Do not print log when FNULL is devnull
        print (output.content)
        print ('Response: %s ms' %(output.elapsed.microseconds/1000))

    if output.json().get("status") == 'error':
        return '-1'

    return output.json().get("data")

def upload(bin):
    url = DestUrl+'/upload/image'
    print ('Uploading \'%s\' to \'%s\' ...' % (bin, url))

    try:
        file = open(bin, 'rb')
        output = requests.post(url, headers={'Content-Type': 'application/octet-stream'}, data=file, verify=False, timeout=900)
    except (requests.exceptions.ConnectionError,
            requests.exceptions.Timeout,
            requests.exceptions.TooManyRedirects,
            requests.exceptions.RequestException,
            requests.exceptions.HTTPError) as e:
        print('Requests exceptions :')
        print (e)
        return ''

    if FNULL is None:  # Do not print log when FNULL is devnull
        print (output.content)
        print ('Response: %s ms' %(output.elapsed.microseconds/1000))

    if output.status_code != requests.codes.ok:
        print (output.content)
        sys.exit(0)

    return output.json().get("data")

def removeManifest():
    try:
        if(os.path.exists('MANIFEST')):
            os.remove('MANIFEST')
    except OSError as e:
        print(e)

def genUploadFile(bin,machine,version,purpose,autoSelect):
    
    if tarfile.is_tarfile(bin):
        print ('Tar file detected, please use unpressed raw binary.')
        sys.exit(1)

    basename = os.path.basename(bin)

    # Remove previous file to be safe
    removeManifest()

    machineWrite=''
    versionWrite='%s' %(basename)
    purposeWrite=''

    if machine!='':
        machineWrite = machine

    if version!='':
        versionWrite = version

    if purpose=='cpld':
        purposeWrite='xyz.openbmc_project.Software.Version.VersionPurpose.CPLD'
    if purpose=='activebios':
        purposeWrite='xyz.openbmc_project.Software.Version.VersionPurpose.Host'
    if purpose=='backupbios':
        purposeWrite='xyz.openbmc_project.Software.Version.VersionPurpose.Host2'
    if purpose=='backupbmc':
        purposeWrite='xyz.openbmc_project.Software.Version.VersionPurpose.BMC2'
    if purpose=='activebmc':
        purposeWrite='xyz.openbmc_project.Software.Version.VersionPurpose.BMC'
    if purpose=='psu1':
        purposeWrite='xyz.openbmc_project.Software.Version.VersionPurpose.PSU'
    if purpose=='psu2':
        purposeWrite='xyz.openbmc_project.Software.Version.VersionPurpose.PSU2'
    if (purpose=='vrMemABCD' or
       purpose=='vrMemEFGH' or
       purpose=='vrCPU' or
       purpose=='vrSOC' or
       purpose=='f7a_p0_vddcr_soc' or
       purpose=='f7a_p0_vdd11' or
       purpose=='f7a_p0_vddio' ) :
        purposeWrite='xyz.openbmc_project.Software.Version.VersionPurpose.VR'

    f = open('MANIFEST', 'w')
    f.write('purpose=%s\n' %(purposeWrite))
    f.write('version=%s\n' %(versionWrite))
    f.write('MachineName=%s\n' %(machineWrite))
    f.close()

    if autoSelect:
        bootSource=getCurrentBmcBootSource()
        if(purpose=='activebmc'):
            if(bootSource==0):
                image="image-bmc"
            elif(bootSource==1):
                image="update_backupbmc_image"
            else:
                autoSelect=False
        elif(purpose=='backupbmc'):
            if(bootSource==0):
                image="update_backupbmc_image"
            elif(bootSource==1):
                image="image-bmc"
            else:
                autoSelect=False

    with tarfile.open('%s.tar' %(basename), 'w') as uploadtarfile:
        if purpose=='activebmc':
            if autoSelect :
                uploadtarfile.add(bin, arcname="%s" %(image))
            else :
                uploadtarfile.add(bin, arcname="image-bmc")
        elif purpose=='backupbmc':
            if autoSelect :
                uploadtarfile.add(bin, arcname="%s" %(image))
            else :
                uploadtarfile.add(bin, arcname="update_%s_image" %(purpose))
        elif ( purpose=='activebios'or
               purpose=='backupbios'or
               purpose=='psu1'or
               purpose=='psu2') :
            uploadtarfile.add(bin, arcname="update_%s_image" %(purpose))
        elif purpose=='vrMemABCD' :
            uploadtarfile.add(bin, arcname="update_VR_MEM_ABCD_image")
        elif purpose=='vrMemEFGH' :
            uploadtarfile.add(bin, arcname="update_VR_MEM_EFGH_image")
        elif purpose=='vrCPU' :
            uploadtarfile.add(bin, arcname="update_VR_CPU_image")
        elif purpose=='vrSOC' :
            uploadtarfile.add(bin, arcname="update_VR_SOC_image")
        elif purpose=='f7a_p0_vddcr_soc' :
            uploadtarfile.add(bin, arcname="update_P0_VDDCR_SOC_image")
        elif purpose=='f7a_p0_vdd11' :
            uploadtarfile.add(bin, arcname="update_P0_VDD11_image")
        elif purpose=='f7a_p0_vddio' :
            uploadtarfile.add(bin, arcname="update_P0_VDDIO_image")
        elif purpose=='cpld' :
            if basename.find("jed") > 0 :
                uploadtarfile.add(bin, arcname="update_cpld_image.jed")
            if basename.find("jbc") > 0 :
                uploadtarfile.add(bin, arcname="update_cpld_image.jbc")
        else:
            uploadtarfile.add(bin, arcname=basename.replace("-",""))
        uploadtarfile.add('MANIFEST')

    removeManifest()

    return ('%s.tar' %(basename))

def update(file,purpose):
    print ('Purpose : %s' %(purpose))
    print ('Update...')

    if (purpose=='activebmc' or purpose=='backupbmc') :
        selectBmcDevice="Not support auto-select BMC device.\nUsing current select update device."
        bootSource=getCurrentBmcBootSource()
        if(purpose=='activebmc'):
            if(bootSource==0):
                selectBmcDevice="Select BMC devcie: CS0"
            elif(bootSource==1):
                purpose = 'backupbmc'
                selectBmcDevice="Select BMC devcie: CS0"
        elif(purpose=='backupbmc'):
            if(bootSource==0):
                selectBmcDevice="Select BMC devcie: CS1"
            elif(bootSource==1):
                purpose = 'activebmc'
                selectBmcDevice="Select BMC devcie: CS1"
        print ('%s' %(selectBmcDevice))


    print ('Upload via REST...')
    versionId = upload(file)

    activeImage(versionId)

    powerOffFlag=1
    powerOffWaitFlag=0
    prepareFlag=0
    programFlag=0
    bmcRebootFlag=0
    verificationFlag=0
    initialFlag=0
    recoveryFlag=0
    displayFlag=1
    timer=0
    dot=''

    if purpose=='cpld':
        while 1 :
            progress=getUpdateProgress(versionId)

            additionalInfo=getUpdateAdditionalInfo(versionId)
            if additionalInfo != "-1" :
                print ('%s' %(additionalInfo))

            if powerOffFlag and int(progress) >= 10 :
                print ('Starting system power off...')
                print ('Waiting for system power off...')
                powerOffFlag = 0
                powerOffWaitFlag=1
            if powerOffWaitFlag and int(progress) >= 30 :
                print ('Starting to erase and program image...')
                powerOffWaitFlag = 0
                programFlag=1
            if programFlag and int(progress) >= 80 :
                print ('Starting to verify image...')
                programFlag=0
                verificationFlag=1
            if int(progress) == -1 :
                if verificationFlag :
                    print ('Updating progress 100%... finished')
                else :
                    print ('Updating progress failed')
                break

            if timer >= updateTimeout:
                print ('Updating progress failed')
                break

            if timer%2 :
                dot = '.'
            else :
                dot = ''
            print ('Updating progress %s%%....%s' %(progress, dot))
            deleteLastLine()
            sleep(1)
            timer = timer + 1
        pass
    if purpose=='activebios' or purpose=='backupbios':
        while 1 :
            progress=getUpdateProgress(versionId)

            additionalInfo=getUpdateAdditionalInfo(versionId)
            if additionalInfo != "-1" :
                print ('%s' %(additionalInfo))

            if powerOffFlag and int(progress) >= 10 :
                print ('Starting to setting the updating mode...')
                powerOffFlag = 0
                prepareFlag=1
            if prepareFlag and int(progress) >= 30 :
                print ('Starting to erase and program image...')
                prepareFlag = 0
                programFlag=1
            if programFlag and int(progress) >= 70 :
                print ('Starting to verify image...')
                programFlag=0
                verificationFlag=1
            if verificationFlag and int(progress) >= 90 :
                print ('Starting to setting the normal mode...')
                verificationFlag=0
                recoveryFlag=1
            if int(progress) == -1 :
                if recoveryFlag :
                    print ('Updating progress 100%... finished')
                else :
                    print ('Updating progress failed')
                break

            if timer >= updateTimeout:
                print ('Updating progress failed')
                break

            if timer%2 :
                dot = '.'
            else :
                dot = ''
            print ('Updating progress %s%%....%s' %(progress, dot))
            deleteLastLine()
            sleep(1)
            timer = timer + 1
        pass
    if purpose=='activebmc':
        progress=20
        programFlag=1
        bmcRebootFlag=1
        while 1 :
            if programFlag and int(progress) >= 20 :
                if displayFlag :
                    print ('Starting to erase,program and verify image...')
                    displayFlag=0
                if bmcRebootFlag :
                    setBmcReboot()
                    bmcRebootFlag=0
                progress=20 + int(timer/3)
                if(progress  >= 90) :
                    programFlag=0
                    initialFlag=1
                    displayFlag=1
            if initialFlag and int(progress) >= 90 :
                if displayFlag :
                    print ('Checking bmc initializing...')
                    print ('Updating progress %s%%....%s' %(progress, dot))
                    deleteLastLine()
                    displayFlag=0
                if(progress  <= 95) :
                    progress=20 + int(timer/3)
                if checkBmcAlive('yes'):
                    initialFlag = 0
                    recoveryFlag=1
            if int(progress) != -1 and recoveryFlag :
                if recoveryFlag :
                    print ('Updating progress 100%... finished')
                else :
                    print ('Updating progress failed')
                break

            if timer >= updateTimeout:
                print ('Updating progress failed')
                break

            if timer%2 :
                dot = '.'
            else :
                dot = ''
            print ('Updating progress %s%%....%s' %(progress, dot))
            deleteLastLine()
            sleep(1)
            timer = timer + 1
        pass
    if purpose=='backupbmc':
        while 1 :
            progress=getUpdateProgress(versionId)

            additionalInfo=getUpdateAdditionalInfo(versionId)
            if additionalInfo != "-1" :
                print ('%s' %(additionalInfo))

            if powerOffFlag and int(progress) >= 30 :
                print ('Starting to erase and program image...')
                powerOffFlag = 0
                programFlag=1
            if programFlag and int(progress) >= 70 :
                print ('Starting to verify image...')
                programFlag=0
                recoveryFlag=1
            if int(progress) == -1 :
                if recoveryFlag :
                    print ('Updating progress 100%... finished')
                else :
                    print ('Updating progress failed')
                break

            if timer >= updateTimeout:
                print ('Updating progress failed')
                break

            if timer%2 :
                dot = '.'
            else :
                dot = ''
            print ('Updating progress %s%%....%s' %(progress, dot))
            deleteLastLine()
            sleep(1)
            timer = timer + 1
        pass
    if purpose=='psu1' or purpose=='psu2':
        while 1 :
            progress=getUpdateProgress(versionId)

            additionalInfo=getUpdateAdditionalInfo(versionId)
            if additionalInfo != "-1" :
                print ('%s' %(additionalInfo))

            if powerOffFlag and int(progress) >= 10 :
                print ('Starting system power off...')
                print ('Waiting for system power off...')
                powerOffFlag = 0
                powerOffWaitFlag=1
            if powerOffWaitFlag and int(progress) >= 30 :
                print ('Starting to erase and program image...')
                powerOffWaitFlag = 0
                programFlag=1
            if programFlag and int(progress) >= 90 :
                print ('Starting to set PSU to normal mode...')
                programFlag=0
                verificationFlag=1
            if int(progress) == -1 :
                if verificationFlag :
                    print ('Updating progress 100%... finished')
                else :
                    print ('Updating progress failed')
                break

            if timer >= updateTimeout:
                print ('Updating progress failed')
                break

            if timer%2 :
                dot = '.'
            else :
                dot = ''
            print ('Updating progress %s%%....%s' %(progress, dot))
            deleteLastLine()
            sleep(1)
            timer = timer + 1
    if (purpose=='vrMemABCD' or
       purpose=='vrMemEFGH' or
       purpose=='vrCPU' or
       purpose=='vrSOC' or
       purpose=='f7a_p0_vddcr_soc' or
       purpose=='f7a_p0_vdd11' or
       purpose=='f7a_p0_vddio' ) :
        while 1 :
            progress=getUpdateProgress(versionId)

            additionalInfo=getUpdateAdditionalInfo(versionId)
            if additionalInfo != "-1" :
                print ('%s' %(additionalInfo))

            if powerOffFlag and int(progress) >= 10 :
                print ('Starting to set and check the updating configuration...')
                powerOffFlag = 0
                prepareFlag=1
            if prepareFlag and int(progress) >= 30 :
                print ('Starting to program image...')
                prepareFlag = 0
                programFlag=1
            if programFlag and int(progress) >= 80 :
                print ('Starting to verify image...')
                programFlag=0
                verificationFlag=1
            if verificationFlag and int(progress) >= 90 :
                print ('Starting to check updating process successfully...')
                verificationFlag=0
                recoveryFlag=1
            if int(progress) == -1 :
                if recoveryFlag :
                    print ('Updating progress 100%... finished')
                else :
                    print ('Updating progress failed')
                break

            if timer >= updateTimeout:
                print ('Updating progress failed')
                break

            if timer%2 :
                dot = '.'
            else :
                dot = ''
            print ('Updating progress %s%%....%s' %(progress, dot))
            deleteLastLine()
            sleep(1)
            timer = timer + 1
        pass
    if purpose!='activebmc':
        print ('Starting to delete upload image...')
        deleteImage(versionId)

def updatePfr(args):
    """Update image by PFR.

    Args:
      args: An object holding attributes.

    Returns:
      The update PFR results.

    """
    # Convert string to boolean.
    args['pfr_reset'] = args['pfr_reset'] == 'true'
    args['pfr_default'] = args['pfr_default'] == 'true'

    # Check a image file is from the TFTP.
    isTftp = args['file'].startswith('tftp://')
    type_json = 'application/json'
    type_multipart = 'multipart/form-data'

    # Gather data from the PFR parameter composition.
    dict_data = {'UpdateRegion': args['pfr_region'],
                'UpdateAtReset': args['pfr_reset'],
                'FactoryDefault': args['pfr_default']}

    try:
        if isTftp:  # Simple Update for TFTP.
            dict_data['ImageURI'] = args['file']  # Add field 'ImageURI' if source is TFTP.
            url = DestUrl + '/redfish/v1/UpdateService/Actions/UpdateService.SimpleUpdate'
            output = requests.post(url, headers={'Content-Type': 'application/json'},
                                   json=dict_data, verify=False, timeout=900)

        else:       # Multiple part update for local file.
            full_path = os.path.abspath(args['file'])
            files = {
                'UpdateParameters': (None, json.dumps(dict_data), 'application/json'),
                'UpdateFile': ('@' + full_path, open(full_path, 'rb'), 'application/octet-stream')
            }
            url = DestUrl + '/redfish/v1/UpdateService/multipartUpdate'
            output = requests.post(url, files=files, verify=False, timeout=900)

    except Exception as e:
        print('Requests exceptions :\n{}'.format(e))
        return False

    if FNULL is None:  # Do not print log when FNULL is devnull
        print ("URL: {}".format(url))
        print ("Path: {}".format(full_path if isTftp else args['file']))
        print ("Data :\n{}".format(json.dumps(dict(dict_data), indent=3)))
        print ("Status Code: {}".format(output.status_code))
        print ("RedFish Response Data :\n{}\n".format(json.dumps(output.json(), indent=3)))
        print ('Response: {} ms'.format(output.elapsed.microseconds/1000))

    if output.status_code != 202: # 202 is accepted.
        print ("RedFish Response Data :\n{}\n".format(json.dumps(output.json(), indent=3)))
        return False

    return True

def main():
    parser = argparse.ArgumentParser(
        description='Upload firmware image file by REST api and update it to purpose device', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-f', '--file', required=True, dest='file',
                        help='The firmware image file to upload and update')
    parser.add_argument('-b', '--bmc', dest='bmc',
                        help='The BMC IP address')
    parser.add_argument('-d', '--device', required=True, dest='device',
                        help='The updating device ( activebmc, backupbmc, activebios, backupbios, cpld, psu1, psu2, pfr )')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Print verbose log')
    parser.add_argument('-u', '--user', help='User name',dest='usr', default='root')
    parser.add_argument('-p', '--password', help='User password',dest='pw', default='root')
    parser.add_argument('-m', '--machine', help='Optionally specify the target machine name of this image.',
                        dest='machine', default='')
    parser.add_argument('-g', '--generate', help='Optionally generate mode to generate specified device tarball',
                        action='store_true')
    parser.add_argument('-pu', '--pfrUpdateRegion', choices=('Active', 'Recovery'), const='active', nargs='?', 
                        dest='pfr_region', default='Active',
                        help='Optionally specify the region to update the PFR image.')
    parser.add_argument('-pr', '--pfrUpdateAtReset', choices=('true', 'false'), const='true', nargs='?',
                        dest='pfr_reset', default=False,
                        help='Optionally, manually power cycle the BMC to continue the update PFR process.')
    parser.add_argument('-pd', '--pfrFactoryDefault', choices=('true', 'false'), const='true', nargs='?',
                        dest='pfr_default', default=False,
                        help='Optionally, Update firmware and restore to default settings.')
    parser.add_argument('-V', '--Version', help='MCT firmware updater tool version',
                        action='version', version='MCT firmware Updater Tool Version : ' + utilityVersion)

    args = parser.parse_args()
    # print(args)
    args = vars(args)

    print('MCT Firmware Updater [v%s]' %(utilityVersion))

    # Disable request warning
    requests.packages.urllib3.disable_warnings()

    if args['generate'] == False:
        if args['file'] is None or args['bmc'] is None or  args['device'] is None :
            parser.print_help()
            sys.exit(1)
        global DestUrl
        DestUrl = 'https://'+args['usr']+':'+args['pw']+'@'+args['bmc']
        print (DestUrl)
        global FNULL
    if args['verbose']:
        FNULL = None  # Print log to stdout/stderr, for debug purpose
    else:
        FNULL = open(os.devnull, 'w')  # Redirect stdout/stderr to devnull

    if path.exists(args['file'])==False :
        if args['device'] != 'pfr' and not args['file'].startswith('tftp://'):
            print ('Please check that the file exists')
            sys.exit(1)

    if (args['device']!='activebios' and
        args['device']!='backupbios' and
        args['device']!='cpld' and
        args['device']!='activebmc' and
        args['device']!='backupbmc' and
        args['device']!='psu1' and
        args['device']!='psu2'and
        args['device']!='vrMemABCD'and
        args['device']!='vrMemEFGH'and
        args['device']!='vrCPU'and
        args['device']!='vrSOC'and
        args['device']!='pfr'and
        args['device']!='f7a_p0_vddcr_soc'and
        args['device']!='f7a_p0_vdd11'and
        args['device']!='f7a_p0_vddio') :
        print ('Invalid device for updating purpose')
        sys.exit(1)

    if (args['device']=='cpld' and
        args['file'].find("jed")==-1 and
        args['file'].find("jbc")==-1) :
        print ('Invalid format for ipnut file. Please input the file with jed or jbc foramt')
        sys.exit(1)

    if args['generate'] == False:
        if checkBmcAlive('no'):
            print ('BMC is alive')
        else:
            print ('BMC is down, check it first')
            sys.exit(1)

    if args['generate'] == True:
        # PFR not support tarball.
        if args['device'] == 'pfr':
            print ('PFR not support generate tarball')
            sys.exit(1)
        else:    
            genUploadFile(args['file'],args['machine'],'',args['device'], False)
            print ('Generate tarball Completed!')
            sys.exit(0)

    if (args['device'] == 'pfr') :
        result = 1
        if updatePfr(args) == True:
            result = 0 # Show success message.
            mesg = ['Hardware will now securely update the firmware image.',
                    'The BMC will not be available for several minutes.',
                    '********** Do not remove the system power **********']
            if args['pfr_reset'] == True:
                mesg.insert(0, 'Manually power cycle the BMC to continue the update PFR process.\n')
            print('\n' + '\n'.join(mesg))
        sys.exit(result)

    elif ( args['device']=='activebios' or
         args['device']=='backupbios' or
         args['device']=='cpld' or
         args['device']=='backupbmc' or
         args['device']=='activebmc' or
         args['device']=='psu1' or
         args['device']=='psu2'or
         args['device']=='vrMemABCD'or
         args['device']=='vrMemEFGH'or
         args['device']=='vrCPU'or
         args['device']=='vrSOC'or
         args['device']=='f7a_p0_vddcr_soc'or
         args['device']=='f7a_p0_vdd11'or
         args['device']=='f7a_p0_vddio') :
        tarfile = genUploadFile(args['file'],args['machine'],'',args['device'], True)
        update(tarfile,args['device'])

    try:
        basename = os.path.basename(args['file'])
        if args['device'] in deviceList and tarfile != basename:
            os.remove('%s.tar' %(basename))
    except OSError as e:
        print(e)

    print ('Completed!')

if __name__ == '__main__':
    main()
