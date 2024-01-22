#
# These file add version 

# Machine name and BMC version for release
MACHINE_NAME = "SV310A"
BMC_VERSION = "1.3.0"

# Replace PRETTY_NAME to add MACHINE_NAME and BMC_VERSION
PRETTY_NAME = "${MACHINE_NAME} ${BMC_VERSION}! (Base: ${DISTRO_NAME} ${VERSION})"
VERSION_ID = "${BMC_VERSION}"
VERSION = "${BMC_VERSION}"

def run_git(d, cmd):
        try:
                oeroot = d.getVar('COREBASE', True)
                return 0
        except:
                pass


# Ensure the git commands run every time bitbake is invoked.
BB_DONT_CACHE = "1"

# Make os-release available to other recipes.
SYSROOT_DIRS_append = " ${sysconfdir}"
