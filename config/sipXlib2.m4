# common directories and variables used in sipxecs
AC_PREFIX_DEFAULT([/usr/local/sipx])
# ${prefix} need double bash resolution to be useful, resolve it once here
eval sf_prefix="${prefix}"

AC_SUBST(SIPX_INCDIR, [${includedir}])
AC_SUBST(SIPX_LIBDIR, [${libdir}])
AC_SUBST(SIPX_LIBEXECDIR, [${libexecdir}/sipXecs])
AC_SUBST(SIPX_BINDIR,  [${bindir}])
AC_SUBST(SIPX_CONFDIR, [${sysconfdir}/sipxpbx])
AC_SUBST(SIPX_DATADIR, [${datadir}/sipxecs])
AC_SUBST(SIPX_INTEROP, [${datadir}/sipxecs/interop])
AC_SUBST(SIPX_DOCDIR,  [${datadir}/doc/sipxecs])
AC_SUBST(SIPX_JAVADIR, [${datadir}/java/sipXecs])
AC_SUBST(SIPX_VARDIR,  [${localstatedir}/sipxdata])
AC_SUBST(SIPX_TMPDIR,  [${localstatedir}/sipxdata/tmp])
AC_SUBST(SIPX_DBDIR,   [${localstatedir}/sipxdata/sipdb])
AC_SUBST(SIPX_LOGDIR,  [${localstatedir}/log/sipxpbx])
AC_SUBST(SIPX_RUNDIR,  [${localstatedir}/run/sipxpbx])
AC_SUBST(SIPX_VARLIB,  [${localstatedir}/lib/sipxpbx])
# sipx RPMs should be hardcoded to use sipxchange user for their sipx user, not the buildbot user
AC_SUBST(SIPX_RPM_CONFIGURE_OPTIONS,  [SIPXPBXUSER=sipxchange])

# Get the user to run sipX under.
AC_ARG_VAR(SIPXPBXUSER, [The sipX service daemon user name, default is ${USER}])
test -n "$SIPXPBXUSER" || SIPXPBXUSER=$USER

# Get the group to run sipX under.
AC_ARG_VAR(SIPXPBXGROUP, [The sipX service daemon group name, default is value of SIPXPBXUSER])
test -n "$SIPXPBXGROUP" || SIPXPBXGROUP=$SIPXPBXUSER
