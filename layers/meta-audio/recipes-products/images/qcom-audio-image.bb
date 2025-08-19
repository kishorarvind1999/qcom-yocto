require recipes-products/images/qcom-console-image.bb

LICENSE = "BSD-3-Clause-Clear"

SUMMARY = "Audio compression dependencies" 

IMAGE_INSTALL:append = " liblc3 libopus codec packagegroup-core-buildessential" 
IMAGE_INSTALL:remove = " fluoride"