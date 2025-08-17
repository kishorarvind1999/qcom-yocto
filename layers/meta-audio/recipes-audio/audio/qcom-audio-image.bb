#require ${LAYERDIR_meta-qcom-distro}/recipes-products/images/qcom-console-image.bb

LICENSE = "BSD-3-Clause-Clear"

SUMMARY = "Audio compression dependencies" 

IMAGE_INSTALL:append = " packagegroup-core-buildessential" 