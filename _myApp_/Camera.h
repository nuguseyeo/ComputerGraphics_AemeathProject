#pragma once

#include <vmath.h>

/// 카메라가 어떤 방식으로 캐릭터를 바라보는지 나타내는 모드.
enum class CameraViewMode {
    /// 캐릭터 뒤에서 orbit 거리만큼 떨어진 카메라.
    ThirdPerson,
    /// 캐릭터 눈높이 근처에서 바라보는 카메라.
    FirstPerson
};

/// 렌더링용 view/projection 계산에 필요한 카메라 상태.
/// ThirdPersonCameraController가 입력으로 갱신한 값을 ModelDrawParams를 통해 전달한다.
struct CameraState {
    /// 수평 회전각(degrees). 0도는 -Z 방향을 기준으로 한다.
    float yawDegrees = 0.0f;
    /// 수직 회전각(degrees). 마우스 Y 입력에 따라 변경된다.
    float pitchDegrees = 0.0f;
    /// 3인칭일 때 타겟으로부터 떨어질 거리. 1인칭에서는 eye 계산에 사용하지 않는다.
    float distance = 0.0f;
    /// true면 first-person 계산 경로를 사용하고, false면 third-person orbit 계산 경로를 사용한다.
    bool firstPerson = false;
};

/// 캐릭터 렌더링 한 프레임에 필요한 카메라 계산 결과.
/// Model::draw는 이 값을 받아 shader uniform과 first-person clipping에 사용한다.
struct CharacterCameraFrame {
    /// perspective projection matrix. FOV/near 값은 1인칭과 3인칭에 따라 다르다.
    vmath::mat4 projection = vmath::mat4::identity();
    /// look-at view matrix. eye에서 target을 바라보도록 계산된다.
    vmath::mat4 view = vmath::mat4::identity();
    /// 월드 공간 카메라 위치. lighting의 viewPos uniform에도 사용된다.
    vmath::vec3 eye = vmath::vec3(0.0f, 0.0f, 0.0f);
    /// 카메라가 바라보는 월드 공간 지점.
    vmath::vec3 target = vmath::vec3(0.0f, 0.0f, 0.0f);
    /// 로딩된 모델 bounds 기반 높이. first-person eye/clip 계산에 재사용된다.
    float modelHeight = 0.0f;
    /// 1인칭에서 얼굴/상체가 화면을 가리지 않도록 shader clipping에 넘기는 최대 Y.
    float firstPersonChestClipY = 0.0f;
    /// false면 캐릭터 mesh draw를 생략한다. 1인칭에서 pitch가 충분히 내려가지 않았을 때 사용한다.
    bool drawCharacterMesh = true;
};

/// view/projection 계산 전담 helper.
/// 렌더링 파이프라인과 OpenGL 상태를 직접 만지지 않고 순수하게 행렬/좌표만 계산한다.
class Camera {
public:
    /// 캐릭터 위치와 모델 bounds를 기준으로 한 프레임의 카메라 행렬과 clipping 값을 만든다.
    /// @param camera yaw/pitch/distance/firstPerson 상태.
    /// @param aspect 현재 window width / height.
    /// @param characterPosition 캐릭터의 월드 위치.
    /// @param modelMinY 모델 로컬 bounds의 최소 Y.
    /// @param modelMaxY 모델 로컬 bounds의 최대 Y.
    /// @param hasModelBounds bounds가 유효하면 true, 없으면 fallback 높이를 사용한다.
    /// @return shader uniform과 draw 조건에 사용할 계산 결과 묶음.
    static CharacterCameraFrame buildCharacterFrame(
        const CameraState& camera,
        float aspect,
        const vmath::vec3& characterPosition,
        float modelMinY,
        float modelMaxY,
        bool hasModelBounds);
};
