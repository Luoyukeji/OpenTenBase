# The Postgres-XL make files exploit features of GNU make that other
# makes do not have. Because it is a common mistake for users to try
# to build Postgres with a different make, we have this make file
# that, as a service, will look for a GNU make and invoke it, or show
# an error message if none could be found.

# If the user were using GNU make now, this file would not get used
# because GNU make uses a make file named "GNUmakefile" in preference
# to "Makefile" if it exists. Postgres-XL is shipped with a
# "GNUmakefile". If the user hasn't run the configure script yet, the
# GNUmakefile won't exist yet, so we catch that case as well.

译文（仅供参考）：
# Postgres-XL 的 make 文件利用了 GNU make 的一些特性，而这些特性在其他 make 工具中并不存在。
# 因为用户尝试使用不同的 make 工具来构建 Postgres 是一个常见的错误，所以我们有这个 make 文件，
# 作为一项服务，它会寻找 GNU make 并调用它，或者如果找不到则显示错误消息。

# 如果用户现在正在使用 GNU make，那么这个文件不会被使用，
# 因为 GNU make 会优先使用名为 "GNUmakefile" 的 make 文件而不是 "Makefile"（如果存在的话）。
# Postgres-XL 附带了一个 "GNUmakefile"。
# 如果用户还没有运行配置脚本，那么 GNUmakefile 还不存在，因此我们也捕获了这种情况。

all check install installdirs installcheck installcheck-parallel uninstall clean distclean maintainer-clean dist distcheck world check-world install-world installcheck-world:
	@if [ ! -f GNUmakefile ] ; then \
	   echo "You need to run the 'configure' program first. See the file"; \
	   echo "'INSTALL' for installation instructions." ; \
	   false ; \
	 fi
	@IFS=':' ; \
	 for dir in $$PATH; do \
	   for prog in gmake gnumake make; do \
	     if [ -f $$dir/$$prog ] && ( $$dir/$$prog -f /dev/null --version 2>/dev/null | grep GNU >/dev/null 2>&1 ) ; then \
	       GMAKE=$$dir/$$prog; \
	       break 2; \
	     fi; \
	   done; \
	 done; \
	\
	 if [ x"$${GMAKE+set}" = xset ]; then \
	   echo "Using GNU make found at $${GMAKE}"; \
	   unset MAKEFLAGS; unset MAKELEVEL; \
	   $${GMAKE} $@ ; \
	 else \
	   echo "You must use GNU make to build Postgres-XL." ; \
	   false; \
	 fi
