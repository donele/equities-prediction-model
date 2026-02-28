set(SOURCES companiesrp.cpp HNewsComp.cpp HNewsRead.cpp HNewsScope.cpp HRavenPack.cpp HRavenPackAgg.cpp
    HRavenPackAggMarketRet.cpp HRavenPackRead.cpp)

add_library(HNews ${SOURCES})
link_directories(${PROJECT_SOURCE_DIR})
