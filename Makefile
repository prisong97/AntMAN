C_FILES := $(shell find ./AntMAN/src -name \*.\*pp -not -name RcppExports.cpp)
R_FILES := $(shell find ./AntMAN/R ./AntMAN/tests -name \*.R -not -name RcppExports.R)

R_CMD := R -q


all : AntMAN.Rinstall/AntMAN/libs/AntMAN.so  tests_cpp/testAntMAN  AntMAN.pdf 

docker : Dockerfile
	mkdir -p docker_share
	sudo rm docker_share/* -rf
	chcon -Rt svirt_sandbox_file_t  docker_share/
	cp AntMAN Makefile new_tutorial.R tests_cpp/ docker_share/ -rf
	sudo docker build -f Dockerfile.3.4.4 -t bbodin/antman344 .
	sudo docker run -v `pwd`/docker_share:/tmp/mixture bbodin/antman344 


test :  AntMAN.Rinstall/AntMAN/libs/AntMAN.so  tests_cpp/testAntMAN  AntMAN.pdf
	${R_CMD} -f AntMAN/tests/testWordCount.R
	${R_CMD} -f AntMAN/tests/testGalaxy.R	
	${R_CMD} -f AntMAN/tests/testSegmentation.R
	${R_CMD} -f new_tutorial.R
	./tests_cpp/testAntMAN

infos :
	@echo "C_FILES=${C_FILES}"
	@echo "R_FILES=${R_FILES}"


check : AntMAN/src/RcppExports.cpp
	${R_CMD} -e  "devtools::check(\"AntMAN\");"

%/NAMESPACE : %/R/AntManAPI.R %/DESCRIPTION
	rm -f $*/man/*
	echo "# Generated by roxygen2: do not edit by hand" > $*/NAMESPACE
	${R_CMD} -e  "library(devtools) ; document(\"$*\");"

%/src/RcppExports.cpp  %/R/RcppExports.R : % %/NAMESPACE ${C_FILES} ${R_FILES}
	rm -f $*/src/RcppExports.cpp  $*/R/RcppExports.R
	${R_CMD} -e  "Rcpp::compileAttributes(pkgdir = \"$*\" , verbose=TRUE);"


%_1.0.tar.gz : ${C_FILES} ${R_FILES} %/src/RcppExports.cpp  %/R/RcppExports.R  
	rm -rf AntMAN/src/*.o ./AntMAN/src/*.so 
	R CMD build ./$*

%.Rcheck/ : ${C_FILES} ${R_FILES} %/src/RcppExports.cpp  %/R/RcppExports.R  
	R CMD check ./$*

%.Rinstall/AntMAN/libs/AntMAN.so : %_1.0.tar.gz 
	mkdir -p $*.Rinstall
	R CMD INSTALL  -l $*.Rinstall $*_1.0.tar.gz

%_1.0.pdf : %/NAMESPACE
	${R_CMD} -e  "library(devtools) ; devtools::build_manual(\"$*\"); " || ${R_CMD} -e  "library(devtools) ; devtools::check(\"$*\",manual=TRUE); " || touch $@

%.pdf : %/NAMESPACE
	R CMD Rd2pdf $* --no-preview --force

tests_cpp/testAntMAN :
	make -C tests_cpp/ testAntMAN

deps :
	echo "To be defined."

clean : 
	rm -rf current *~ *.Rinstall *.pdf  *_1.0.tar.gz *.Rcheck ./AntMAN/NAMESPACE ./AntMAN/src/*.o ./AntMAN/src/*.so 	./AntMAN/src/*.rds ./AntMAN/src/RcppExports.cpp  ./AntMAN/R/RcppExports.R  ./AntMAN/man/AM*.Rd tests_cpp/testAntMAN

.PHONY: clean
.SECONDARY:
