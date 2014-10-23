SUMMARY = "AT command deamon tool"
SECTION = "gnokii"
LICENSE = "GPL-2.0"
LIC_FILES_CHKSUM = "file://COPYING;md5=cda9de7a272071e08b00d9885865873b"

S = "${WORKDIR}/gnokii"

SRC_URI = "file://gnokii"

FILES_${PN} = "${libdir} ${bindir} ${sbindir}"

inherit autotools gettext

EXTRA_OECONF = " --disable-nls --disable-mysql --disable-libusb"

do_configure_prepend() {
    sh ${S}/autogen.sh
    mv -f ${S}/configure.ac ${S}/configure.ac.old
}

do_configure_append() {
    rm -f ${S}/missing
    mv -f ${S}/configure.ac.old ${S}/configure.ac
    touch ${S}/missing
}

do_package_qa() {
}