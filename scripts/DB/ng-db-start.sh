#!/bin/sh

# product   : NewsGate - news search WEB server
# copyright : Copyright (c) 2005-2016 Karen Arutyunov
# licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
#             Commercial; contact karen.arutyunov@gmail.com

ng-mysqld status > /dev/null

if test $? -eq 0; then 
  echo "MySQL DB instance is running";
else

  if test ! -d "$NEWSGATE_DBDIR/mysql"; then 
    echo "Installing MySQL DB ..."

    ng-db-install

    if test $? -eq 0; then 
      echo "done" 
      echo ""
    else 
      echo "failed"; 
      exit 1
    fi

  else
    echo "MySQL DB is already installed"
  fi

  echo -n "Starting MySQL DB instance ... "

  ng-mysqld start > /dev/null

  if test $? -ne 0; then 
    echo "failed"; 
    exit 1
  fi

  succeeded=0

  for i in 0 1 2 4 8 16 32; do 

    if test $i -ne 0; then
      sleep $i
    fi

    if test -S "$MYSQL_WORKSPACE_ROOT/mysql.socket"; then 
      succeeded=1
      break
    fi
  done

  if test $succeeded -eq 0; then 
    echo "failed"
    exit 1
  fi

  echo "done"
  echo ""
fi

echo "select count(*) from Event" | \
ng-mysql --user root NewsGate > /dev/null 2>&1

if test $? -ne 0; then 
  ng-db-prepare

  if test $? -ne 0; then 
    echo "failed";
    exit 1
  fi

  echo ""

else
  echo "NewsGate DB is ready"
fi

echo "select count(*) from ModeratorPrivileges" | \
ng-mysql --user root NewsGateModeration > /dev/null 2>&1

if test $? -ne 0; then 
  ng-moddb-prepare

  if test $? -ne 0; then 
    echo "failed";
    exit 1
  fi
else
  echo "NewsGateModeration DB is ready"
fi

exit 0 
