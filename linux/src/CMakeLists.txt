set(sources 
    main.cpp
    webserver.cpp
    utils.cpp
    eventloop.cpp
    http.cpp
    channel.cpp
    httprequest.cpp
    httpresponse.cpp
    httpproxy.cpp
)

add_executable(main ${sources})
target_link_libraries(main pthread)
