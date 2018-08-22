default:
	@echo "Try make stable, make unstable, make tuto-s4u or make push."
	@echo "Also possible: DOCKER_EXTRA=--no-cache make unstable"

stable:
	export last_tag=`wget https://framagit.org/simgrid/simgrid/tags 2>/dev/null -O - | grep /simgrid/simgrid/tags/v | head -n1  | sed 's/[^>]*>//' | sed 's/<.*//'`; \
	export url=`wget https://framagit.org/simgrid/simgrid/tags/$${last_tag} 2>/dev/null -O - | grep SimGrid- | perl -pe 's/.*?<li><a href="//' | sed 's/tar.gz.*/tar.gz/'` ;\
	echo URL:$${url} ; \
	docker build -f Dockerfile.stable \
	         --build-arg DLURL=$${url} \
	         -t simgrid/stable:latest \
                 -t simgrid/stable:$${last_tag} \
		 $(DOCKER_EXTRA) \
                 . | tee > stable.log

unstable:
	docker build -f Dockerfile.unstable \
	         -t simgrid/unstable:latest \
                 -t simgrid/unstable:$$(date --iso-8601) \
		 $(DOCKER_EXTRA) \
                 . | tee > unstable.log

tuto-s4u: 
	docker build -f Dockerfile.tuto-s4u \
	         -t simgrid/tuto-s4u:latest \
                 -t simgrid/tuto-s4u:$$(date --iso-8601) \
		 $(DOCKER_EXTRA) \
                 . | tee > tuto.log

push:
	docker push simgrid/stable
	docker push simgrid/unstable
	docker push simgrid/tuto-s4u
