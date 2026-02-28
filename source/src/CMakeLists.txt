#if("$ENV{HOSTNAME}" STREQUAL "smrc-chi22.ronin.roncap.com")
#set(SUBDIRS jl_lib gtlib gtlib_fastsim gtlib_fitana gtlib_fitting gtlib_predana
#gtlib_signal gtlib_sigread gtlib_linmod gtlib_tickdump gtlib_model
#MCore MFitObj MFitMod MFitting MFramework MSigMod MSignal)
#else()
#set(SUBDIRS jl_lib gtlib gtlib_fastsim gtlib_fitana gtlib_fitting gtlib_predana
#gtlib_signal gtlib_sigread gtlib_gui gtlib_linmod gtlib_tickdump gtlib_model
#HCore HLib HOrders HTickSeries
#MCore MFitObj MFitMod MFitting MFramework MSigMod MSignal)
#endif()

set(SUBDIRS jl_lib gtlib gtlib_fastsim gtlib_fitana gtlib_fitting gtlib_predana
	gtlib_signal gtlib_sigread gtlib_linmod gtlib_tickdump gtlib_model
	MCore MFitObj MFitMod MFitting MFramework MOrders MSigMod MSignal)

if(NOT "$ENV{HOSTNAME}" STREQUAL "smrc-chi22.ronin.roncap.com")
	#set(SUBDIRS ${SUBDIRS} gtlib_gui HCore HLib HOrders HTickSeries)
	set(SUBDIRS ${SUBDIRS} HCore HLib HOrders HTickSeries)
endif()

foreach(lib ${SUBDIRS})
    add_subdirectory(${lib})
endforeach(lib)
