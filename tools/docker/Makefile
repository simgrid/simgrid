default:
	@echo "Existing Targets:"
	@echo "   make build-deps -> build an image with all possible SimGrid dependencies"
	@echo "   make stable     -> build the latest stable version of SimGrid (with SMPI w/o MC)"
	@echo "   make unstable   -> build the git version of SimGrid (with SMPI, w/o MC)"
	@echo "   make tuto-s4u   -> build all what you need to take the S4U tutorial"
	@echo "   make tuto-smpi  -> build all what you need to take the SMPI tutorial"
	@echo "   make tuto-mc    -> build the git version of SimGrid (with SMPI and MC)"
	@echo "   make all        -> build all but stable (ie, build-deps unstable tuto-s4u tuto-smpi)"
	@echo "   make push       -> push all images to the cloud"
	@echo "All our images are based on debian:testing"
	@echo "Also possible: DOCKER_EXTRA=--no-cache make unstable"

all: build-deps unstable tuto-s4u tuto-smpi

stable:
	# docker build -f Dockerfile.stable --build-arg DLURL=/simgrid/simgrid/-/archive/v3.28/simgrid-v3.28.tar.gz -t simgrid/stable:latest -t simgrid/stable:v3.25 . |tee stable.log
	export last_tag=$$(wget https://framagit.org/simgrid/simgrid/tags 2>/dev/null -O - | grep /simgrid/simgrid/-/tags/v | head -n1  | sed 's/[^>]*>//' | sed 's/<.*//'); \
 	echo "DLURL=/simgrid/simgrid/-/archive/$${last_tag}/simgrid-$${last_tag}.tar.gz";\
	docker build -f Dockerfile.stable \
	         --build-arg DLURL=$${DLURL} \
		 -t simgrid/stable:latest \
                 -t simgrid/stable:"/simgrid/simgrid/-/archive/$${last_tag}/simgrid-$${last_tag}.tar.gz" \
		 $(DOCKER_EXTRA) \
                 . | tee stable.log

unstable:
	docker build -f Dockerfile.unstable \
	         -t simgrid/unstable:latest \
                 -t simgrid/unstable:$$(date --iso-8601) \
		 $(DOCKER_EXTRA) \
                 . | tee unstable.log

build-deps:
	docker build -f Dockerfile.build-deps \
	         -t simgrid/build-deps:latest \
                 -t simgrid/build-deps:$$(date --iso-8601) \
		 $(DOCKER_EXTRA) \
                 . | tee build-deps.log

tuto-mc:
	docker build -f Dockerfile.tuto-mc \
	         -t simgrid/tuto-mc:latest \
                 -t simgrid/tuto-mc:$$(date --iso-8601) \
		 $(DOCKER_EXTRA) \
                 . | tee tuto-mc.log

build-deps-stable:
	docker build -f Dockerfile.build-deps-stable \
	         -t simgrid/build-deps-stable:latest \
                 -t simgrid/build-deps-stable:$$(date --iso-8601) \
		 $(DOCKER_EXTRA) \
                 . | tee build-deps-stable.log

tuto-s4u:
	docker build -f Dockerfile.tuto-s4u \
	         -t simgrid/tuto-s4u:latest \
                 -t simgrid/tuto-s4u:$$(date --iso-8601) \
		 $(DOCKER_EXTRA) \
                 . | tee tuto-s4u.log

tuto-smpi:
	docker build -f Dockerfile.tuto-smpi \
	         -t simgrid/tuto-smpi:latest \
                 -t simgrid/tuto-smpi:$$(date --iso-8601) \
		 $(DOCKER_EXTRA) \
                 . | tee tuto-smpi.log

push:
	docker push simgrid/build-deps
	docker push simgrid/stable
	docker push simgrid/unstable
	docker push simgrid/tuto-s4u
	docker push simgrid/tuto-smpi
	docker push simgrid/tuto-mc
