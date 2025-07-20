idf_component_register(SRCS
  "src/BLEBroadcaster.cpp"
  "src/BLEEmitter.cpp"
  "src/BLEEmitterContainer.cpp"
  "src/BLEGenericEmitter.cpp"
  "src/BLEDummyEmitter.cpp"
  "src/BLEStrings.cpp"
  INCLUDE_DIRS "include"
  REQUIRES bt nvs_flash)

