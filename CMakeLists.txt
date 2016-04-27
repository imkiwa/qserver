cmake_minimum_required (VERSION 2.6)

include_directories (include)

add_library  (qserver
    src/qserver.c
    src/qserver-extension.c
  )

install (
  TARGETS qserver
  
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install (
  DIRECTORY include
  
  DESTINATION .
  
  PATTERN "*.h"
  PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ
)
