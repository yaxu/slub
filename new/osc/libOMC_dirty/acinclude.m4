dnl
dnl Check for minimum version of libtool
dnl AC_PREREQ_LIBTOOL([MINIMUM VERSION],[ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]])
AC_DEFUN([AC_PREREQ_LIBTOOL],
  [
    lt_min_full=ifelse([$1], ,1.3.5,$1)
    lt_min=`echo $lt_min_full | sed -e 's/\.//g'`
    AC_MSG_CHECKING(for libtool >= $lt_min_full)
    lt_version="`grep '^VERSION' $srcdir/ltmain.sh | sed -e 's/VERSION\=//g;s/[[-.a-zA-Z]]//g'`"

    if test $lt_version -lt 100 ; then
      lt_version=`expr $lt_version \* 10`
    fi

    if test $lt_version -lt $lt_min ; then
      AC_MSG_RESULT(no)
      ifelse([$3], , :, [$3])
    fi
    AC_MSG_RESULT(yes)
    ifelse([$2], , :, [$2])
  ])

