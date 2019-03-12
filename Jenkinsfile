pipeline {
  agent { label 'vaskemaskin' }

  environment {
    PROFILE_x86_64 = 'clang-6.0-linux-x86_64'
    PROFILE_x86 = 'clang-6.0-linux-x86'
    CPUS = """${sh(returnStdout: true, script: 'nproc')}"""
    INCLUDEOS_PREFIX = "${env.WORKSPACE}/install"
    CC = 'clang-6.0'
    CXX = 'clang++-6.0'
    USER = 'includeos'
    CHAN = 'test'
    MOD_VER= '0.13.0'
    REMOTE = 'includeos-test'
    COVERAGE_DIR = "${env.COVERAGE_DIR}/${env.JOB_NAME}"
  }

  stages {
    stage('Setup') {
      steps {
        sh 'mkdir -p install'
        sh 'cp conan/profiles/* ~/.conan/profiles/'
      }
    }

    stage('Pull Request pipeline') {
      when { changeRequest() }
      stages {
        stage('Unit tests') {
          steps {
            sh script: "mkdir -p unittests", label: "Setup"
            sh script: "cd unittests; env CC=gcc CXX=g++ cmake ../test", label: "Cmake"
            sh script: "cd unittests; make -j $CPUS", label: "Make"
            sh script: "cd unittests; ctest", label: "Ctest"
          }
        }
        stage('liveupdate x86_64') {
          steps {
            //This ordering is wrong and should come post building includeos package
            build_liveupdate_package("$PROFILE_x86_64")
          }
        }
        stage('mana x86_64') {
          steps {
          	build_editable('lib/mana','mana')
          }
        }
        stage('mender x86_64') {
          steps {
          	build_editable('lib/mender','mender')
          }
        }
        stage('uplink x86_64') {
          steps {
          	build_editable('lib/uplink','uplink')
          }
        }
        stage('microLB x86_64') {
          steps {
          	build_editable('lib/microLB','microlb')
          }
        }
        /* TODO
        stage('build chainloader 32bit') {
          steps {
        sh """
              cd src/chainload
            rm -rf build || :&& mkdir build
            cd build
            conan link .. chainloader/$MOD_VER@$USER/$CHAN --layout=../layout.txt
            conan install .. -pr $PROFILE_x86 -u
            cmake --build . --config Release
        """
          }
        }
        */
        stage('Build & Integration tests') {
          stages {
            stage('Build 32 bit') {
              steps {
                sh script: "mkdir -p build_x86", label: "Setup"
                sh script: "cd build_x86; cmake -DCONAN_PROFILE=$PROFILE_x86 -DARCH=i686 -DPLATFORM=x86_nano ..", label: "Cmake"
                sh script: "cd build_x86; make -j $CPUS", label: "Make"
                sh script: 'cd build_x86; make install', label: "Make install"
              }
            }
            stage('Build 64 bit') {
              steps {
                sh script: "mkdir -p build_x86_64", label: "Setup"
                sh script: "cd build_x86_64; cmake -DCONAN_PROFILE=$PROFILE_x86_64 ..", label: "Cmake"
                sh script: "cd build_x86_64; make -j $CPUS", label: "Make"
                sh script: "cd build_x86_64; make install", label: "Make install"
              }
            }
            stage('Build examples') {
              steps {
          	    sh script: "mkdir -p build_examples", label: "Setup"
                sh script: "cd build_examples; cmake ../examples", label: "Cmake"
                sh script: "cd build_examples; make -j $CPUS", label: "Make"
              }
             }
            stage('Integration tests') {
              steps {
                sh script: "mkdir -p integration", label: "Setup"
                sh script: "cd integration; cmake ../test/integration -DSTRESS=ON, -DCMAKE_BUILD_TYPE=Debug", label: "Cmake"
                sh script: "cd integration; make -j $CPUS", label: "Make"
                sh script: "cd integration; ctest -E stress --output-on-failure", label: "Tests"
                sh script: "cd integration; ctest -R stress -E integration --output-on-failure", label: "Stress test"
              }
            }
          }
        }
        stage('Code coverage') {
          steps {
            sh script: "mkdir -p coverage; rm -r $COVERAGE_DIR || :", label: "Setup"
            sh script: "cd coverage; env CC=gcc CXX=g++ cmake -DCOVERAGE=ON -DCODECOV_HTMLOUTPUTDIR=$COVERAGE_DIR ../test", label: "Cmake"
            sh script: "cd coverage; make -j $CPUS", label: "Make"
            sh script: "cd coverage; make coverage", label: "Make coverage"
          }
          post {
            success {
              script {
                if (env.CHANGE_ID) {
                  pullRequest.comment("Code coverage: ${env.COVERAGE_ADDRESS}/${env.JOB_NAME}")
                }
              }
            }
          }
        }
      }
    }

    stage('Dev branch pipeline') {
      when {
        anyOf {
          branch 'master'
          branch 'dev'
        }
      }
      stages {
        stage('Build Conan package') {
          steps {
            build_conan_package("$PROFILE_x86", "ON")
            build_conan_package("$PROFILE_x86_64")
            build_liveupdate_package("$PROFILE_x86_64")
          }
        }
        stage('Upload to bintray') {
          steps {
            script {
              def version = sh (
                script: 'conan inspect -a version . | cut -d " " -f 2',
                returnStdout: true
              ).trim()
              sh script: "conan upload --all -r $REMOTE includeos/${version}@$USER/$CHAN", label: "Upload includeos to bintray"
              sh script: "conan upload --all -r $REMOTE liveupdate/${version}@$USER/$CHAN", label: "Upload liveupdate to bintray"
            }
          }
        }
      }
    }
  }
  post {
    cleanup {
      sh script: """
        VERSION=\$(conan inspect -a version lib/LiveUpdate | cut -d " " -f 2)
        conan remove liveupdate/\$VERSION@$USER/$CHAN -f || echo 'Could not remove. This does not fail the pipeline'
      """, label: "Cleaning up and removing conan package"
    }
  }
}

def build_editable(String location, String name) {
  sh """
    cd $location
    mkdir -p build
    cd build
    conan link .. $name/$MOD_VER@$USER/$CHAN --layout=../layout.txt
    conan install .. -pr $PROFILE_x86_64 -u
    cmake -DARCH=x86_64 ..
    cmake --build . --config Release
  """
}

def build_conan_package(String profile, basic="OFF") {
  sh script: "conan create . $USER/$CHAN -pr ${profile} -o basic=${basic}", label: "Build with profile: $profile"
}

def build_liveupdate_package(String profile) {
  sh script: "conan create lib/LiveUpdate $USER/$CHAN -pr ${profile}", label: "Build with profile: $profile"
}