cmake_minimum_required (VERSION 3.16)
project (forward7)

set(CMAKE_CXX_STANDARD 20)

#
find_package(Qt6 REQUIRED COMPONENTS
Core
Gui
Widgets
StateMachine
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

#
include(FetchContent)

FetchContent_Declare(
  pcap-pp
  GIT_REPOSITORY https://github.com/seladb/PcapPlusPlus.git
  GIT_TAG        master
)

FetchContent_MakeAvailable(pcap-pp)

set(FORWARD_SRC
main.cpp

mainwindow.h
mainwindow.cpp

addruledialog.h
addruledialog.cpp

forward_rule.h

forwardworker.h
forwardworker.cpp
)
list(TRANSFORM FORWARD_SRC PREPEND src/)

qt6_add_resources(FORWARD_SRC src/resources.qrc)

#
add_executable (forward7 ${FORWARD_SRC})

target_include_directories(forward7 PRIVATE 
${pcap-pp_SOURCE_DIR}/Pcap++/header
${pcap-pp_SOURCE_DIR}/Packet++/header
${pcap-pp_SOURCE_DIR}/Common++/header
)

target_link_directories(forward7 PRIVATE 
${pcap-pp_BINARY_DIR}/Pcap++
${pcap-pp_BINARY_DIR}/Common++
)

#
set(QT_LIBS
Qt6::Core 
Qt6::Gui 
Qt6::Widgets 
Qt6::StateMachine
)

set(PCAP-PP_LIBS
Pcap++ 
Common++
)

target_link_libraries(forward7 PRIVATE ${QT_LIBS} ${PCAP-PP_LIBS})
