all: test

clean:
	rm -f $(PROG) *~
	rm -rf tmp-geonames-db
	rm -f cnf/geonames.conf


TEST_PREPARE_TARGETS := cnf/geonames.conf
TEST_RUNNER_OPT :=

TEST_DB_DIR := $(shell pwd)/tmp-geonames-db

ifdef CI
GEONAMES_HOST_EDIT := sed -e 's|"smartmet-test"|"$(TEST_DB_DIR)"|g'
TEST_PREPARE_TARGETS += geonames-database start-geonames-db
TEST_RUNNER_OPT += --ignore .expect_to_fail
TEST_FINISH_TARGETS := stop-test-db
else
GEONAMES_HOST_EDIT := cat
TEST_FINISH_TARGETS := dummy
endif

TEST_RUNNER :=


test: $(TEST_PREPARE_TARGETS)
	@rm -f failures/*
	@echo Running tests:
	ok=true; $(TEST_RUNNER) smartmet-plugin-test --handler=/admin --reactor-config=cnf/reactor.conf $(TEST_RUNNER_OPT) || ok=false; $(MAKE) $(TEST_FINISH_TARGETS); $$ok

geonames-database:
	@-$(MAKE) stop-test-db
	rm -rf tmp-geonames-db
	if ! /usr/share/smartmet/test/db/create-local-db.sh $(TEST_DB_DIR) >tmp-geonames-db.log 2>&1 ; then \
	    cat tmp-geonames-db.log; \
	    false; \
	fi

start-geonames-db:
	/usr/share/smartmet/test/db/test-db-ctl.sh $(TEST_DB_DIR) start -w

stop-test-db:
	-/usr/share/smartmet/test/db/test-db-ctl.sh $(TEST_DB_DIR) stop

cnf/geonames.conf:	dummy
	$(GEONAMES_HOST_EDIT) $@.in >$@

dummy:
