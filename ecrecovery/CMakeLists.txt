add_executable( ecrecovery
    ecrecovery.cpp
    ecrecoveryparams.cpp
    ecrecoveryhandler.cpp
)

target_link_libraries( ecrecovery
    CgiCC
    PostgreSQL
    cam
    evbusutil
    eventcache
    failover
    failovernvrstate
    mediadatabase
    sqlite3
    veutil
)


CopyFilesTo( "${TARGET_ROOT_BIN}/ve-script/" "UnitNotificationRecoveryStream" )

