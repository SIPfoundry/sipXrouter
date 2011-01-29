#!/bin/sh
##
## upgrade-cert.sh
##
##  Copyright (C) 2009 Pingtel Corp., certain elements licensed under a Contributor Agreement.  
##  Contributors retain copyright to elements licensed under a Contributor Agreement.
##  Licensed to the User under the LGPL license.

################################################################
## This script is used during an upgrade from 3.10 to 
## copy certificate and key files from the 3.10 location to the
## new location and then install them.
##
## See the comments at the top of gen-ssl-keys.sh for the list 
## of files in the WorkDir.
################################################################

GenDir="/home/joegen/sipxecs-bin/var/sipxdata"
OldGenDir=${HOME}
sipXuser="joegen"
sipXgroup="joegen"

#   external tools
openssl="/usr/bin/openssl"


set -e
if [ -d "${OldGenDir}/sipx-certdb" ]
then
   # old SSL working directory found.
   echo "Migrating SSL certificate Database to new location"
   if ! [ -d "${GenDir}/certdb" ]
   then
      # create the directory.
      mkdir  -v  "${GenDir}/certdb"
   fi
   cd "${OldGenDir}/sipx-certdb/"
   cp -v -r "." "${GenDir}/certdb" 

   cd "${GenDir}/certdb"
   rm -rfv "${OldGenDir}/sipx-certdb/"

   echo "Changing permissions on new Certificate Database files"
   chown -vR ${sipXuser}   "${GenDir}/certdb" 
   chgrp -vR ${sipXgroup}   "${GenDir}/certdb" 
   chmod -vR 0700          "${GenDir}/certdb" 

   # install the certificates.
   echo "Installing certificates for the host"
   /home/joegen/sipxecs-bin/bin/ssl-cert/install-cert.sh "`hostname --fqdn`" 
   exit 0
fi

