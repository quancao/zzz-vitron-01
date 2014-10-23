# Adapted from linux-imx.inc, copyright (C) 2012, 2013 O.S. Systems Software LTDA
# Released under the MIT license (see COPYING.MIT for the terms)

SUMMARY = "Linux kernel for Wandboard"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"

inherit kernel

# Put a local version until we have a true SRCREV to point to
#LOCALVERSION ?= "+yocto"
#SCMVERSION ?= "y"

SRC_URI = "file://linux-controlboard \
           file://defconfig \
"

S = "${WORKDIR}/linux-controlboard"

# We need to pass it as param since kernel might support more then one
# machine, with different entry points
KERNEL_EXTRA_ARGS += "LOADADDR=${UBOOT_ENTRYPOINT}"

kernel_conf_variable() {
        CONF_SED_SCRIPT="$CONF_SED_SCRIPT /CONFIG_$1[ =]/d;"
        if test "$2" = "n"
        then
                echo "# CONFIG_$1 is not set" >> ${S}/.config
        else
                echo "CONFIG_$1=$2" >> ${S}/.config
        fi
}

do_configure_prepend() {
        echo "" > ${S}/.config
        CONF_SED_SCRIPT=""

        kernel_conf_variable LOCALVERSION "\"${LOCALVERSION}\""
        kernel_conf_variable LOCALVERSION_AUTO y

        sed -e "${CONF_SED_SCRIPT}" < '${WORKDIR}/defconfig' >> '${S}/.config'

        if [ "${SCMVERSION}" = "y" ]; then
                # Add GIT revision to the local version
                head=`git rev-parse --verify --short HEAD 2> /dev/null`
                printf "%s%s" +g $head > ${S}/.scmversion
        fi
}


# Put a local version until we have a true SRCREV to point to
LOCALVERSION ?= "controlboard"
#SCMVERSION ?= "y"

#SRCBRANCH ??= "master"

# Allow override of WANDBOARD_GITHUB_MIRROR to make use of
# local repository easier
CONTROLBOARD_GITHUB_MIRROR ?= "git://github.com/wandboard-org/linux.git"

# SRC_URI for wandboard kernel
#SRC_URI = "file://linux-controlboard"

