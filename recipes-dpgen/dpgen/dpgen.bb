SUMMARY = "Digital pattern generator for Intel Galileo"
SECTION = "misc"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
  file://main.c \
  file://printer.h \
  file://printer.c \
  file://pattern.h \
  file://pattern.c \
  file://arg_parser.h \
  file://arg_parser.c \
  file://pattern/arecibo \
  file://pattern/barker13 \
  file://pattern/pi \
  file://pattern/square \
  "

DEPENDS = "mraa"

S = "${WORKDIR}"

# TODO: Consider adding a Makefile
do_compile() {
  ${CC} ${LDFLAGS} -c arg_parser.c
  ${CC} ${LDFLAGS} -c printer.c
  ${CC} ${LDFLAGS} -c pattern.c
  ${CC} ${LDFLAGS} arg_parser.o printer.o pattern.o main.c -o dpgen -lmraa
}

do_install() {
  install -d ${D}${bindir}
  install -m 0755 dpgen ${D}${bindir}
  install -d ${D}${datadir}
  install -d ${D}${datadir}/dpgen
  install -d ${D}${datadir}/dpgen/pattern
  install pattern/arecibo ${D}${datadir}/dpgen/pattern
  install pattern/barker13 ${D}${datadir}/dpgen/pattern
  install pattern/pi ${D}${datadir}/dpgen/pattern
  install pattern/square ${D}${datadir}/dpgen/pattern
}
