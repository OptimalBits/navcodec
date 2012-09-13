ALL_TESTS = $(shell find test -name '*.test.js')
REPORTER = spec
UI = bdd

all: clean build test

test:
	@./node_modules/.bin/mocha \
                --require should \
                --reporter $(REPORTER) \
                --ui $(UI) \
                --growl \
                $(ALL_TESTS)

clean: 
	@node-gyp clean
	@rm -rf test/fixtures/transcode/*

build: 
	@node-gyp configure build 

.PHONY: test
