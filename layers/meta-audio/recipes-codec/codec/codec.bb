SUMMARY = "Audio Codec Test"
SECTION = "code"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://codec.c \
    file://input.pcm \
"

S = "${WORKDIR}"

DEPENDS += " libopus liblc3"

do_compile() {
         ${CC} ${CFLAGS} ${LDFLAGS} codec.c -o codec -lopus -llc3 -lm
}

do_install() {
         install -d ${D}${bindir}
         install -m 0755 codec ${D}${bindir}

         install -d ${D}${datadir}/${PN}
         install -m 0644 ${WORKDIR}/input.pcm ${D}${datadir}/${PN}/
}