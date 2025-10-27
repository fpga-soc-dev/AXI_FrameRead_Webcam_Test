LICENSE = "CLOSED"

SRC_URI = "\
    file://CMakeLists.txt \
    file://main.cpp \
    file://BarcodeDetector.cpp \
    file://BarcodeDetector.hpp \
    file://BarcodeProcessor.hpp \
    file://BarcodeProcessor.cpp \
    file://CameraPipeline.hpp \
    file://CameraPipeline.cpp \
    file://Config.hpp \
    file://FrameBufferDisplay.hpp \
    file://FrameBufferDisplay.cpp \
    "

S = "${WORKDIR}"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
do_rm_work[noexec] = "1"

DEPENDS += " spdlog opencv gstreamer1.0  gstreamer1.0-plugins-base zxing-cpp"

inherit cmake pkgconfig

do_install() {

    install -d ${D}/home/root
    install -m 0755 ${B}/BarcodeReader ${D}/home/root/

}


FILES:${PN} += "/home/root/BarcodeReader"
