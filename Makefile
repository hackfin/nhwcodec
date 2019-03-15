test:
	$(MAKE) -C TEST clean all-test testdiff

diff:
	$(MAKE) -C TEST clean all-test diff

.PHONY: test

%:
	$(MAKE) -C encoder $*
	$(MAKE) -C decoder $*
