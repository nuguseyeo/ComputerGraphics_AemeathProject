#pragma once

#include <string>
#include <vector>
#include <vmath.h>

// SkeletonNode는 Assimp 노드 계층을 모델 내부 포맷으로 복사한 트리 노드입니다.
struct SkeletonNode {
    std::string name;
    vmath::mat4 baseTransform = vmath::mat4::identity();
    std::vector<SkeletonNode> children;
};