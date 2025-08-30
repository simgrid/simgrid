#! /bin/sh -x

image_name=$1
dockerfile=$2

if [ -e /etc/debian_version ] ; then
  date_arg="-d"
else # alpine
  date_arg="-D"
fi

if docker pull $image_name ; then 
	git_time=`git log -n1 --format=%at $dockerfile` ;
	docker_time=`docker inspect -f '{{.Created}}' $(docker images -q $image_name) | xargs -I {} date $date_arg {} +%s` ;
	if [ $docker_time -lt $git_time ] ; then
		need_build=yes ;
	fi
else 
	need_build=yes ;
fi
need_build=yes
if [ -n "$need_build" ] ; then
	docker build -f $dockerfile -t $image_name . ;
	docker push $image_name ; 
fi