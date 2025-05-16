idf_component_register(SRCS
  "blab.cpp"
  "BLEBroadcaster.cpp"
  "BLEEmitter.cpp"
  "BLEDummyEmitter.cpp"
  REQUIRES bt nvs_flash
  INCLUDE_DIRS "." "include"
)

# This is the default
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++23)

add_compile_options(-Wno-missing-field-initializers)

