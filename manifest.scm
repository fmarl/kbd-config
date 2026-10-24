(use-modules
 (gnu packages cross-base)
 (gnu packages embedded))

(concatenate-manifests
 (list
  (packages->manifest
   (list
    (make-arm-none-eabi-nano-toolchain-12.3.rel1)))
  (specifications->manifest
   (list
    "qmk"
    "imagemagick"
    "make"
    "gcc-toolchain"))))
