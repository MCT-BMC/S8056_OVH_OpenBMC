#
# These file add version 

python() {
        d.setVar('VERSION', "v0.29-00-s8033")
        d.setVar('VERSION_ID', "v0.29-00-s8033")
        d.setVar('BMC_NAME', "GC70-B8033")
}

OS_RELEASE_FIELDS:append = "BMC_NAME"

# Ensure the git commands run every time bitbake is invoked.
BB_DONT_CACHE = "1"

# Make os-release available to other recipes.
SYSROOT_DIRS:append = " ${sysconfdir}"
