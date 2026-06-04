cmake_minimum_required(VERSION 3.10)
project(AEM_CLP_BSG CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
  lib/bullet-2.80/src
  metasolver
  metasolver/strategies
  problems/clp
  problems/clp/containers
  problems/clp/evaluators
  problems/clp/objects2
)

link_directories(
  lib/bullet-2.80/src/BulletCollision
  lib/bullet-2.80/src/BulletDynamics
  lib/bullet-2.80/src/LinearMath
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -no-pie")

add_library(bsg
  metasolver/SearchStrategy.cpp
  metasolver/GlobalVariables.cpp
  metasolver/State.cpp
  metasolver/strategies/BSG.cpp
  metasolver/strategies/Greedy.cpp
  metasolver/strategies/BSG_VCS_MCTS.cpp
  metasolver/mcts/DecisionKey.cpp
  metasolver/mcts/MCTS.cpp
  metasolver/mcts/MCTSNode.cpp
  metasolver/mcts/MCTSStats.cpp
)

add_library(clp
  problems/clp/containers/AABBContainer.cpp
  problems/clp/containers/AABBTree.cpp
  problems/clp/evaluators/VCS_Function.cpp
  problems/clp/evaluators/VLossFunction.cpp
  problems/clp/objects2/AABB.cpp
  problems/clp/objects2/Block.cpp
  problems/clp/objects2/Block_fsb.cpp
  problems/clp/objects2/BoxShape.cpp
  problems/clp/objects2/Space.cpp
  problems/clp/objects2/SpaceSet.cpp
)

add_library(clp_state
  problems/clp/clpState.cpp
)

add_executable(BSG_CLP
  problems/clp/main_clp.cpp
)

target_link_libraries(BSG_CLP
  clp_state
  clp
  bsg
  BulletDynamics
  BulletCollision
  LinearMath
)

enable_testing()

add_test(
  NAME bsg_clp_smoke
  COMMAND $<TARGET_FILE:BSG_CLP> problems/clp/benchs/BR/BR4.txt -i 1 -t 1 -f BR
)

set_tests_properties(bsg_clp_smoke PROPERTIES
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
