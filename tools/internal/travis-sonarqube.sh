#! /bin/sh

# Install and run SonarQube on travis. 
#
# Use it as a wrapper to your build command, eg: ./travis-sonarqube.sh make VERBOSE=1

# On Mac OSX or with pull requests, you don't want to run SonarQube but to exec the build command directly.
if [ ${TRAVIS_OS_NAME} != 'linux' ] || [ ${TRAVIS_PULL_REQUEST} != 'false' ] 
then
  exec "$@"
fi
# Passed this point, we are on Linux and not in a PR (exec never returns)


# Be verbose and fail fast
set -ex

# Install required software
installSonarQubeScanner() {
  export SONAR_SCANNER_HOME=$HOME/.sonar/sonar-scanner-2.6
  rm -rf $SONAR_SCANNER_HOME
  mkdir -p $SONAR_SCANNER_HOME
  curl -sSLo $HOME/.sonar/sonar-scanner.zip http://repo1.maven.org/maven2/org/sonarsource/scanner/cli/sonar-scanner-cli/2.6/sonar-scanner-cli-2.6.zip
  unzip $HOME/.sonar/sonar-scanner.zip -d $HOME/.sonar/
  rm $HOME/.sonar/sonar-scanner.zip
  export PATH=$SONAR_SCANNER_HOME/bin:$PATH
  export SONAR_SCANNER_OPTS="-server"
}
installBuildWrapper() {
  curl -LsS https://nemo.sonarqube.org/static/cpp/build-wrapper-linux-x86.zip > build-wrapper-linux-x86.zip
  unzip build-wrapper-linux-x86.zip
}
installSonarQubeScanner
installBuildWrapper

# triggers the compilation through the build wrapper to gather compilation database
./build-wrapper-linux-x86/build-wrapper-linux-x86-64 --out-dir bw-outputs "$@"

# and finally execute the actual SonarQube analysis (the SONAR_TOKEN is set from the travis web interface, to not expose it)
# See https://docs.travis-ci.com/user/sonarqube/ for more info on tokens
sonar-scanner -Dsonar.host.url=https://nemo.sonarqube.org -Dsonar.login=$SONAR_TOKEN
