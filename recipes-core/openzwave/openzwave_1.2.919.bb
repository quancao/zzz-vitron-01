SUMMARY = "AT command deamon tool"
SECTION = "gnokii"
LICENSE = "GPL-2.0"
LIC_FILES_CHKSUM = "file://license/lgpl.txt;md5=b52f2d57d10c4f7ee67a7eb9615d5d24"

S = "${WORKDIR}/openzwave_${PV}"

SRC_URI = "file://openzwave_${PV}"

FILES_${PN} = "${libdir} ${bindir} ${sbindir}"

EXTRA_OEMAKE = 'CROSS_COMPILE=${TARGET_PREFIX}'

do_configure() {
}

do_compile() {
    oe_runmake all
}

do_install() {
	oe_runmake 'DESTDIR=${D}' install
}

do_package_qa() {
}
