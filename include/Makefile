# Makefile wrapper (emacs rules !)
# by benjamin gerard <ben@sashipa.com>
#
# $Id: Makefile,v 1.1 2003-01-21 02:38:16 ben Exp $
#

%: force
	@echo "Wrap target [$@]"
	@$(MAKE) -C ../src $(filter-out,Makefile,$@)
force: ;
