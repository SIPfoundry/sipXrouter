dnl Initial Version Copyright (C) 2010 eZuce, Inc., All Rights Reserved.
dnl Licensed to the User under the LGPL license.
dnl

AC_ARG_WITH(yum-proxy, [--with-yum-proxy send downloads thru caching proxy like squid to speed downloads], [
  AC_SUBST(DOWNLOAD_PROXY,$withval)
  AC_SUBST(DOWNLOAD_PROXY_CONFIG_LINE,"proxy=$withval")

  # Using a BASEURL is instead of yum using mirrorlists to randomize download sites is important if you want
  # to get any use from your caching proxy.
  #
  # Qualifications on selecting BASE_URLs
  #   1. Must support proper cache headers.
  #   2. Reliable
  # Non-qualifications
  #   3. Fastest 
  #
  # If you're using squid, you can test if a site will cache thru your proxy run this command twice, 
  # replacing 'your-proxy-server' appropriately
  #
  #   http_proxy=http://your-proxy-server:3128 wget -S http://mirrors.rit.edu/centos/5.5/os/i386/repodata/repomd.xml
  #
  # and you should see 
  #
  #   X-Cache: MISS from swift.hubler.us
  #
  # go to
  #
  #   X-Cache: HIT from swift.hubler.us
  #
  
  CENTOS_BASE_URL=http://mirrors.rit.edu/centos
  FEDORA_BASE_URL=http://mirrors.rit.edu/fedora/linux
],)

dnl NOTE: To support non-rpm based distos, write equivalent of this that defines DISTRO_* vars
AC_CHECK_FILE(/bin/rpm, 
[
  RPMBUILD_TOPDIR="\$(shell rpm --eval '%{_topdir}')"
  AC_SUBST(RPMBUILD_TOPDIR)

  RPM_DIST=`rpm --eval '%{?dist}'`
  AC_SUBST(RPM_DIST)

  DistroArchDefault=`rpm --eval '%{_target_cpu}'`

  dnl NOTE LIMITATION: default doesn't account for distros besides centos, redhat and suse or redhat 6
  DistroOsDefault=`rpm --eval '%{?fedora:fedora}%{!?fedora:%{?suse_version:suse}%{!?suse_version:centos}}'`

  dnl NOTE LIMITATION: default doesn't account for distros besides centos, redhat and suse or redhat 6
  DistroVerDefault=`rpm --eval '%{?fedora:%fedora}%{!?fedora:%{?suse_version:%suse_version}%{!?suse_version:5}}'`

  DistroDefault="${DistroOsDefault}-${DistroVerDefault}-${DistroArchDefault}"
])

AC_ARG_VAR(DISTRO, [What operating system you are compiling for. Default is ${DistroDefault}])
test -n "${DISTRO}" || DISTRO="${DistroDefault:-centos-5-i386}"

AllDistrosDefault="centos-5-i386 centos-5-x86_64 fedora-14-i386 fedora-14-x86_64"
AC_ARG_VAR(ALL_DISTROS, [All distros which using cross distroy compiling (xc.* targets) Default is ${AllDistrosDefault}])
test -n "${ALL_DISTROS}" || ALL_DISTROS="${AllDistrosDefault}"

AC_ARG_ENABLE(rpm, [--enable-rpm Using mock package to build rpms],
[
  AC_ARG_VAR(DOWNLOAD_LIB_CACHE, [When to cache source files that are downloaded, default ~/libsrc])
  test -n "${DOWNLOAD_LIB_CACHE}" || DOWNLOAD_LIB_CACHE=~/libsrc

  AC_ARG_VAR(DOWNLOAD_LIB_URL, [When to cache source files that are downloaded, default download.sipfoundry.org])
  test -n "${DOWNLOAD_LIB_URL}" || DOWNLOAD_LIB_URL=http://download.sipfoundry.org/pub/sipXecs/libs

  AC_ARG_VAR(RPM_DIST_DIR, [Where to assemble final set of RPMs and SRPMs in preparation for publishing to a download server.])
  test -n "${RPM_DIST_DIR}" || RPM_DIST_DIR=repo

  # Allows you to create centos repos on fedora
  AS_VERSION_COMPARE('0.5.0', [`createrepo --version | awk '{print $NF}'`],
   [BACKWARD_COMPATIBLE_CREATEREPO_OPTS_FOR_CENTOS="--checksum=sha"],,)
  AC_SUBST(BACKWARD_COMPATIBLE_CREATEREPO_OPTS_FOR_CENTOS)

  AC_ARG_VAR(CENTOS_BASE_URL, [Where to find CentOS distribution. Example: http://centos.aol.com])
  if test -n "$CENTOS_BASE_URL"; then
    AC_SUBST(CENTOS_BASE_URL_ON,[])
    AC_SUBST(CENTOS_BASE_URL_OFF,[#])
  else
    AC_SUBST(CENTOS_BASE_URL_ON,[#])
    AC_SUBST(CENTOS_BASE_URL_OFF,[])
  fi

  AC_ARG_VAR(FEDORA_BASE_URL, [Where to find Fedora distribution. Example: http://mirrors.kernel.org/fedora])
  if test -n "$FEDORA_BASE_URL"; then
    AC_SUBST(FEDORA_BASE_URL_ON,[])
    AC_SUBST(FEDORA_BASE_URL_OFF,[#])
  else
    AC_SUBST(FEDORA_BASE_URL_ON,[#])
    AC_SUBST(FEDORA_BASE_URL_OFF,[])
  fi

  AC_CONFIG_FILES([mak/mock/logging.ini])
  AC_CONFIG_FILES([mak/mock/site-defaults.cfg])
  AC_CONFIG_FILES([mak/mock/centos-5-i386.cfg])
  AC_CONFIG_FILES([mak/mock/centos-5-x86_64.cfg])
  AC_CONFIG_FILES([mak/mock/fedora-14-i386.cfg])
  AC_CONFIG_FILES([mak/mock/fedora-14-x86_64.cfg])
  AC_CONFIG_FILES([mak/10-rpm.mk])
])
