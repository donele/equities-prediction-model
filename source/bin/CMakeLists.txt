set(JL_LIBRARIES jl_lib
	gtlib gtlib_fastsim gtlib_fitana gtlib_fitting gtlib_predana gtlib_signal gtlib_sigread
	gtlib_linmod gtlib_tickdump gtlib_model)
set(MM_LIBRARIES tickdata optionclasses)
set(MODEL_LIBRARIES MCore MFitObj MFitMod MFitting MFramework
	MSigMod MSignal MOrders)

if("$ENV{HOSTNAME}" STREQUAL "smrc-chi22.ronin.roncap.com")
	set(model_bins mx)
	set(simple_bins get_idate)
	set(all_bins ${model_bins} ${simple_bins})

	set(COMMON_LIBRARIES ${MM_LIBRARIES} ${JL_LIBRARIES} ${Boost_LIBRARIES})
else()
	set(t_bins testCpp11 testHeap mytest testPreload testString)
	set(simple_bins tickDataCount tickDataDump compPred compSig db
		get_idate merge_us_book)
	set(model_bins mx checkParams sigDump)
	set(h_bins hf)
	#set(root_bins guiTest guiBidAsk)
	set(all_bins ${t_bins} ${simple_bins} ${model_bins} ${h_bins} ${root_bins})

	#set(JLROOT_LIBRARIES gtlib_gui)
	set(H_LIBRARIES HCore HLib HOrders HTickSeries)
	set(COMMON_LIBRARIES ${MM_LIBRARIES} ${JL_LIBRARIES} ${Boost_LIBRARIES} Threads::Threads)
endif()


foreach(bin ${all_bins})
  add_executable(${bin} ${bin}.cpp)
  install(TARGETS ${bin} DESTINATION bin)
endforeach(bin)


foreach(bin ${t_bins} ${simple_bins})
	target_link_libraries(${bin} ${COMMON_LIBRARIES})
endforeach(bin)

foreach(bin ${model_bins})
	target_link_libraries(${bin} ${COMMON_LIBRARIES} ${MODEL_LIBRARIES})
endforeach(bin)

foreach(bin ${h_bins})
	target_link_libraries(${bin} ${COMMON_LIBRARIES} ${MODEL_LIBRARIES} ${H_LIBRARIES} ${JLROOT_LIBRARIES} ${ROOT_LIBRARIES})
endforeach(bin)

foreach(bin ${root_bins})
	target_link_libraries(${bin} ${COMMON_LIBRARIES} ${JLROOT_LIBRARIES} ${ROOT_LIBRARIES})
endforeach(bin)
