# AX_ARG_ENABLE(feature, help-string, defaultval, if-eanble-yes, if-enable-no)
# author: nagadomi@nurs.or.jp
#
AC_DEFUN([AX_ARG_ENABLE],
[AC_ARG_ENABLE($1, $2, 
[case  "${enableval}" in
  yes)
    $4
    ;;
  no)
    $5
    ;;
  *)
   AC_MSG_ERROR([*** bad value ${enableval} for $1])
   ;;
esac
],
[case  "$3" in
  yes)
    $4
    ;;
  no)
    $5
    ;;
  *) AC_MSG_ERROR([*** bad value $3 for $1]) ;;
esac
])])
