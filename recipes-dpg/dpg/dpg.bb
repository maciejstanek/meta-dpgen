SUMMARY = "Digital pattern generator for Intel Galileo"
SECTION = "misc"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://dpg.c"

S = "${WORKDIR}"

do_compile() {
	     ${CC} ${LDFLAGS} dpg.c -o dpg
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 dpg ${D}${bindir}
}
