add_subdirectory(cli)
add_subdirectory(loader)

if(RPLC_XL)
    add_subdirectory(notification)
elseif(RPLC_L)
    add_subdirectory(notification)
elseif(RPLC_M)
    add_subdirectory(notification)
else()
    if(RHS_SERVICE_NOTIFICATION)
        add_subdirectory(notification)
    endif()
endif()
