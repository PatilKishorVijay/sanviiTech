include ../Makefile.defs
include ${PROJECTPATH}/xStore/buildtype

SRCS=ecrecovery.cpp ecrecoveryparams.cpp ecrecoveryhandler.cpp
TARGET=ecrecovery

EXTRA_LIBS += libevbusutil.so.1 \
	$(LIBCAM_DEP_LIBS) \
	$(LIBNVRAUTH_LIBS) \
	libpthread.so.0 \
	$(LIBVEUTIL_LIBS) \
	libruntimestats.so.1 \
	libcgicc.so.5 \
	libcgiutils.so.1 \
	$(LIBINPUTCONFIGUTILS_LIBS) \
	libcameraconfigutils.so.1 \
	libvecommon.so.1 \
	liblog4cpp.so.5.0.6 \
	libeventcache.so.1 \
	libsqliteutil.so.1 \
	libsqlite3.so.0 \
	libfailover.so.1 \
	libmediadatabase.so.1 \
	libfailovernvrstate.so.1 libvejson.so.1 $(JSON_GLIB_LIBS)

INCLUDES= -I. -I$(PROJECTPATH)/xStore/include \
	-I$(PROJECTPATH)/xStore/libevbusutil \
	-I$(PROJECTPATH)/xStore/veapi/libveapi \
	-I$(PROJECTPATH)/xStore/veapi/libveapitext \
	-I$(PROJECTPATH)/xStore/veapi/libveapiutils \
	-I$(PROJECTPATH)/xStore/device_handlers \
	-I$(PROJECTPATH)/xStore/device_handlers/interface \
	-I$(PROJECTPATH)/xStore/libutil \
	-I$(PROJECTPATH)/xStore/libruntimestats \
	-I$(PROJECTPATH)/xStore/libcgiutils \
	-I$(PROJECTPATH)/xStore/libinputconfigutils \
	-I$(PROJECTPATH)/xStore/libcameraconfigutils/db \
	-I$(PROJECTPATH)/xStore/libdrycontactconfigutils \
	-I$(PROJECTPATH)/xStore/libnvrauth \
	-I$(PROJECTPATH)/xStore/libsqliteutil \
	-I$(PROJECTPATH)/xStore/libeventcache \
	-I$(PROJECTPATH)/xStore/libevbusutil \
	-I/usr/include/pgsql \
	-I$(PROJECTPATH)/xStore/failover/libfailovernvrstate \
	-I$(PROJECTPATH)/xStore/libfailover

CPPFLAGS= $(WERROR) $(INCLUDES)
EXTRA_INCLUDES=$(INCLUDES)
# # have override STANDARD_LIBS or we'll get in trouble
# STANDARD_LIBS=
# LDFLAGS=-L$(LIBPATH)

include ${PROJECTPATH}/xStore/Makefile.commondep
include ${PROJECTPATH}/xStore/Makefile.execcpp



