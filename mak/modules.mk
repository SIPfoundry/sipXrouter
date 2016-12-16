# Initial Version Copyright (C) 2010 eZuce, Inc., All Rights Reserved.
# Licensed to the User under the LGPL license.

# sipXecs projects that are essential for a running communication system.
# Order is important, projects that are dependant on other project
# should be listed after it's dependencies.  No circular dependecies
# allowed.
sipx_core = \
  sipXportLib \
  sipXtackLib \
  sipXcommserverLib \
  sipXtools \
  oss_core \
  sipXproxy \
  sipXpublisher \
  sipXregistry \
  sipXkamailio

#additional configure options for sipXresiprocate package
sipXresiprocate_OPTIONS = --with-c-ares --with-ssl --with-repro --enable-ipv6 --with-tfm
oss_core_OPTIONS = --enable-all-features

sipx_all =   $(sipx_core)


# Project compile-time dependencies. Only list project that if
# it's dependecies were recompiled then you'd want to recompile.
sipXtackLib_DEPS = sipXportLib
sipXcommserverLib_DEPS = sipXtackLib 
sipXproxy_DEPS = sipXcommserverLib 
sipXregistry_DEPS = sipXcommserverLib
sipXtools_DEPS = sipXtackLib sipXcommserverLib
sipXkamailio = oss_core sipXcommserverLib

all = $(sipx)
