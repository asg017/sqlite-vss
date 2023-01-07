loadable:
	cmake -B build; make -C build

test:
	python3 tests/test-loadable.py

.PHONY: loadable test