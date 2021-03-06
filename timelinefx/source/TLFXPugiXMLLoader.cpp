#include "TLFXPugiXMLLoader.h"
#include "TLFXAnimImage.h"
#include "TLFXEffect.h"
#include "TLFXEmitter.h"

#include <cassert>

namespace TLFX
{

    bool PugiXMLLoader::Open( const char *filename )
    {
        _error[0] = 0;
        pugi::xml_parse_result result = _doc.load_file(filename);

        if (!result)
        {
            snprintf(_error, sizeof(_error), "Parsing error at #%d : %s", result.offset, result.description());
            return false;
        }
        if (!_doc.child("EFFECTS"))
        {
            snprintf(_error, sizeof(_error), "Root element <EFFECTS> is missing");
            return false;
        }

        _currentShape = _doc.child("EFFECTS").child("SHAPES").child("IMAGE");

        _currentFolder = _doc.child("EFFECTS").child("FOLDER");
        while (!_currentEffect && _currentFolder)
        {
            _currentEffect = _currentFolder.child("EFFECT");
            if (!_currentEffect)                // empty folder
                _currentFolder = _currentFolder.next_sibling("FOLDER");
        }
        if (!_currentEffect)            // no effect in any folder
            _currentEffect = _doc.child("EFFECTS").child("EFFECT");

        return true;
    }

    const char* PugiXMLLoader::GetLastError() const
    {
        return _error;
    }

    bool PugiXMLLoader::GetNextShape( AnimImage *shape )
    {
        _error[0] = 0;

        if (!_currentShape)
        {
            snprintf(_error, sizeof(_error), "No more shapes there");
            return false;
        }

        shape->SetFilename   (_currentShape.attribute("URL").as_string());
        shape->SetWidth      (_currentShape.attribute("WIDTH").as_float());
        shape->SetHeight     (_currentShape.attribute("HEIGHT").as_float());
        shape->SetFramesCount(_currentShape.attribute("FRAMES").as_int());
        shape->SetIndex      (_currentShape.attribute("INDEX").as_int() + _existingShapeCount);
        // name is autogenerated from Filename if left empty

        float maxRadius = _currentShape.attribute("MAX_RADIUS").as_float();
        if (maxRadius != 0)
            shape->SetMaxRadius(maxRadius);
        else
            shape->FindRadius();

        _currentShape = _currentShape.next_sibling("IMAGE");
        return true;
    }

    Effect* PugiXMLLoader::GetNextEffect(const std::list<AnimImage*>& sprites)
    {
        if (!_currentEffect)
        {
            snprintf(_error, sizeof(_error), "No more effects there");
            return NULL;
        }

        Effect *effect;
        if (_currentFolder)
            effect = LoadEffect(_currentEffect, sprites, NULL, _currentFolder.attribute("NAME").as_string());
        else
            effect = LoadEffect(_currentEffect, sprites);


        _currentEffect = _currentEffect.next_sibling("EFFECT");
        if (!_currentEffect)
        {
            if (_currentFolder)
            {
                _currentFolder = _currentFolder.next_sibling("FOLDER");
                _currentEffect = _currentFolder.child("EFFECT");
            }
        }

        return effect;
    }

    Effect* PugiXMLLoader::LoadEffect( pugi::xml_node& node, const std::list<AnimImage*>& sprites, Emitter *parent, const char *folderPath /*= ""*/ )
    {
        Effect *e = new Effect();

        e->SetClass            (node.attribute("TYPE").as_int());
        e->SetEmitAtPoints     (node.attribute("EMITATPOINTS").as_bool());
        e->SetMGX              (node.attribute("MAXGX").as_int());
        e->SetMGY              (node.attribute("MAXGY").as_int());
        e->SetEmissionType     (node.attribute("EMISSION_TYPE").as_int());
        e->SetEllipseArc       (node.attribute("ELLIPSE_ARC").as_float());
        e->SetEffectLength     (node.attribute("EFFECT_LENGTH").as_int());
        e->SetLockAspect       (node.attribute("UNIFORM").as_bool());
        e->SetName             (node.attribute("NAME").as_string());
        e->SetHandleCenter     (node.attribute("HANDLE_CENTER").as_bool());
        e->SetHandleX          (node.attribute("HANDLE_X").as_int());
        e->SetHandleY          (node.attribute("HANDLE_Y").as_int());
        e->SetTraverseEdge     (node.attribute("TRAVERSE_EDGE").as_bool());
        e->SetEndBehavior      (node.attribute("END_BEHAVIOUR").as_int());
        e->SetDistanceSetByLife(node.attribute("DISTANCE_SET_BY_LIFE").as_bool());
        e->SetReverseSpawn     (node.attribute("REVERSE_SPAWN_DIRECTION").as_bool());
        e->SetParentEmitter(parent);

        std::string path;
        if (parent)
            path = parent->GetPath();
        else
            path = folderPath;
        if (!path.empty())
            path += "/";
        path += e->GetName();
        e->SetPath(path.c_str());

        pugi::xml_node animation = node.child("ANIMATION_PROPERTIES");
        if (animation)
        {
            e->SetFrames     (animation.attribute("FRAMES").as_int());
            e->SetAnimWidth  (animation.attribute("WIDTH").as_int());
            e->SetAnimHeight (animation.attribute("HEIGHT").as_int());
            e->SetAnimX      (animation.attribute("X").as_int());
            e->SetAnimY      (animation.attribute("Y").as_int());
            e->SetSeed       (animation.attribute("SEED").as_int());
            e->SetLooped     (animation.attribute("LOOPED").as_bool());
            e->SetZoom       (animation.attribute("ZOOM").as_float());
            e->SetFrameOffset(animation.attribute("LOOPED").as_bool());
        }

        AttributeNode *attr = NULL;
        pugi::xml_node attrnode;

        for (attrnode = node.child("AMOUNT"); attrnode; attrnode = attrnode.next_sibling("AMOUNT"))
        {
            attr = e->AddAmount(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("LIFE"); attrnode; attrnode = attrnode.next_sibling("LIFE"))
        {
            attr = e->AddLife(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("SIZEX"); attrnode; attrnode = attrnode.next_sibling("SIZEX"))
        {
            attr = e->AddSizeX(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("SIZEY"); attrnode; attrnode = attrnode.next_sibling("SIZEY"))
        {
            attr = e->AddSizeY(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("VELOCITY"); attrnode; attrnode = attrnode.next_sibling("VELOCITY"))
        {
            attr = e->AddVelocity(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("WEIGHT"); attrnode; attrnode = attrnode.next_sibling("WEIGHT"))
        {
            attr = e->AddWeight(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("SPIN"); attrnode; attrnode = attrnode.next_sibling("SPIN"))
        {
            attr = e->AddSpin(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("ALPHA"); attrnode; attrnode = attrnode.next_sibling("ALPHA"))
        {
            attr = e->AddAlpha(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("EMISSIONANGLE"); attrnode; attrnode = attrnode.next_sibling("EMISSIONANGLE"))
        {
            attr = e->AddEmissionAngle(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("EMISSIONRANGE"); attrnode; attrnode = attrnode.next_sibling("EMISSIONRANGE"))
        {
            attr = e->AddEmissionRange(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("AREA_WIDTH"); attrnode; attrnode = attrnode.next_sibling("AREA_WIDTH"))
        {
            attr = e->AddWidth(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("AREA_HEIGHT"); attrnode; attrnode = attrnode.next_sibling("AREA_HEIGHT"))
        {
            attr = e->AddHeight(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("ANGLE"); attrnode; attrnode = attrnode.next_sibling("ANGLE"))
        {
            attr = e->AddAngle(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("STRETCH"); attrnode; attrnode = attrnode.next_sibling("STRETCH"))
        {
            attr = e->AddStretch(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }
        
        if (!node.child("STRETCH"))
        {
            e->AddStretch(0, 1.0f);
        }

        for (attrnode = node.child("GLOBAL_ZOOM"); attrnode; attrnode = attrnode.next_sibling("GLOBAL_ZOOM"))
        {
            attr = e->AddGlobalZ(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (pugi::xml_node particle = node.child("PARTICLE"); particle; particle = particle.next_sibling("PARTICLE"))
        {
            e->AddChild(LoadEmitter(particle, sprites, e));
        }

        return e;
    }

    void PugiXMLLoader::LoadAttributeNode( pugi::xml_node& node, AttributeNode* attr )
    {
        for (pugi::xml_node c = node.child("CURVE"); c; c = c.next_sibling("CURVE"))
        {
            attr->SetCurvePoints(c.attribute("LEFT_CURVE_POINT_X").as_float(),
                                 c.attribute("LEFT_CURVE_POINT_Y").as_float(),
                                 c.attribute("RIGHT_CURVE_POINT_X").as_float(),
                                 c.attribute("RIGHT_CURVE_POINT_Y").as_float());
        }
    }

    Emitter* PugiXMLLoader::LoadEmitter( pugi::xml_node& node, const std::list<AnimImage*>& sprites, Effect *parent )
    {
        Emitter* e = new Emitter;

        e->SetHandleX           (node.attribute("HANDLE_X").as_int());
        e->SetHandleY           (node.attribute("HANDLE_Y").as_int());
        e->SetBlendMode         (node.attribute("BLENDMODE").as_int());
        e->SetParticlesRelative (node.attribute("RELATIVE").as_bool());
        e->SetRandomColor       (node.attribute("RANDOM_COLOR").as_bool());
        e->SetZLayer            (node.attribute("LAYER").as_int());
        e->SetSingleParticle    (node.attribute("SINGLE_PARTICLE").as_bool());
        e->SetName              (node.attribute("NAME").as_string());
        e->SetAnimate           (node.attribute("ANIMATE").as_bool());
        e->SetOnce              (node.attribute("ANIMATE_ONCE").as_bool());
        e->SetCurrentFrame      (node.attribute("FRAME").as_float());
        e->SetRandomStartFrame  (node.attribute("RANDOM_START_FRAME").as_bool());
        e->SetAnimationDirection(node.attribute("ANIMATION_DIRECTION").as_int());
        e->SetUniform           (node.attribute("UNIFORM").as_bool());
        e->SetAngleType         (node.attribute("ANGLE_TYPE").as_int());
        e->SetAngleOffset       (node.attribute("ANGLE_OFFSET").as_int());
        e->SetLockAngle         (node.attribute("LOCK_ANGLE").as_bool());
        e->SetAngleRelative     (node.attribute("ANGLE_RELATIVE").as_bool());
        e->SetUseEffectEmission (node.attribute("USE_EFFECT_EMISSION").as_bool());
        e->SetColorRepeat       (node.attribute("COLOR_REPEAT").as_int());
        e->SetAlphaRepeat       (node.attribute("ALPHA_REPEAT").as_int());
        e->SetOneShot           (node.attribute("ONE_SHOT").as_bool());
        e->SetHandleCenter      (node.attribute("HANDLE_CENTERED").as_bool());
        e->SetGroupParticles    (node.attribute("GROUP_PARTICLES").as_bool());

        if (e->GetAnimationDirection() == 0)
            e->SetAnimationDirection(1);

        e->SetParentEffect(parent);
        std::string path = parent->GetPath();
        path = path + "/" + e->GetName();
        e->SetPath(path.c_str());

        pugi::xml_node sub;

        sub = node.child("SHAPE_INDEX");
        if (sub)
            e->SetImage(GetSpriteInList(sprites, atoi(sub.child_value()) + _existingShapeCount));

        sub = node.child("ANGLE_TYPE");
        if (sub)
            e->SetAngleType(sub.attribute("VALUE").as_int());

        sub = node.child("ANGLE_OFFSET");
        if (sub)
            e->SetAngleOffset(sub.attribute("VALUE").as_int());

        sub = node.child("LOCKED_ANGLE");
        if (sub)
            e->SetLockAngle(sub.attribute("VALUE").as_bool());

        sub = node.child("ANGLE_RELATIVE");
        if (sub)
            e->SetAngleRelative(sub.attribute("VALUE").as_bool());

        sub = node.child("USE_EFFECT_EMISSION");
        if (sub)
            e->SetUseEffectEmission(sub.attribute("VALUE").as_bool());

        sub = node.child("COLOR_REPEAT");
        if (sub)
            e->SetColorRepeat(sub.attribute("VALUE").as_int());

        sub = node.child("ALPHA_REPEAT");
        if (sub)
            e->SetAlphaRepeat(sub.attribute("VALUE").as_int());

        sub = node.child("ONE_SHOT");
        if (sub)
            e->SetOneShot(sub.attribute("VALUE").as_bool());

        sub = node.child("HANDLE_CENTERED");
        if (sub)
            e->SetHandleCenter(sub.attribute("VALUE").as_bool());

        AttributeNode* attr = NULL;
        pugi::xml_node attrnode;

        for (attrnode = node.child("LIFE"); attrnode; attrnode = attrnode.next_sibling("LIFE"))
        {
            attr = e->AddLife(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("AMOUNT"); attrnode; attrnode = attrnode.next_sibling("AMOUNT"))
        {
            attr = e->AddAmount(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("BASE_SPEED"); attrnode; attrnode = attrnode.next_sibling("BASE_SPEED"))
        {
            attr = e->AddBaseSpeed(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("BASE_WEIGHT"); attrnode; attrnode = attrnode.next_sibling("BASE_WEIGHT"))
        {
            attr = e->AddBaseWeight(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("BASE_SIZE_X"); attrnode; attrnode = attrnode.next_sibling("BASE_SIZE_X"))
        {
            attr = e->AddSizeX(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("BASE_SIZE_Y"); attrnode; attrnode = attrnode.next_sibling("BASE_SIZE_Y"))
        {
            attr = e->AddSizeY(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("BASE_SPIN"); attrnode; attrnode = attrnode.next_sibling("BASE_SPIN"))
        {
            attr = e->AddBaseSpin(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("SPLATTER"); attrnode; attrnode = attrnode.next_sibling("SPLATTER"))
        {
            attr = e->AddSplatter(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("LIFE_VARIATION"); attrnode; attrnode = attrnode.next_sibling("LIFE_VARIATION"))
        {
            attr = e->AddLifeVariation(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("AMOUNT_VARIATION"); attrnode; attrnode = attrnode.next_sibling("AMOUNT_VARIATION"))
        {
            attr = e->AddAmountVariation(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("VELOCITY_VARIATION"); attrnode; attrnode = attrnode.next_sibling("VELOCITY_VARIATION"))
        {
            attr = e->AddVelVariation(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("WEIGHT_VARIATION"); attrnode; attrnode = attrnode.next_sibling("WEIGHT_VARIATION"))
        {
            attr = e->AddWeightVariation(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("SIZE_X_VARIATION"); attrnode; attrnode = attrnode.next_sibling("SIZE_X_VARIATION"))
        {
            attr = e->AddSizeXVariation(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("SIZE_Y_VARIATION"); attrnode; attrnode = attrnode.next_sibling("SIZE_Y_VARIATION"))
        {
            attr = e->AddSizeYVariation(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("SPIN_VARIATION"); attrnode; attrnode = attrnode.next_sibling("SPIN_VARIATION"))
        {
            attr = e->AddSpinVariation(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("DIRECTION_VARIATION"); attrnode; attrnode = attrnode.next_sibling("DIRECTION_VARIATION"))
        {
            attr = e->AddDirectionVariation(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("ALPHA_OVERTIME"); attrnode; attrnode = attrnode.next_sibling("ALPHA_OVERTIME"))
        {
            attr = e->AddAlpha(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("VELOCITY_OVERTIME"); attrnode; attrnode = attrnode.next_sibling("VELOCITY_OVERTIME"))
        {
            attr = e->AddVelocity(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("WEIGHT_OVERTIME"); attrnode; attrnode = attrnode.next_sibling("WEIGHT_OVERTIME"))
        {
            attr = e->AddWeight(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("SCALE_X_OVERTIME"); attrnode; attrnode = attrnode.next_sibling("SCALE_X_OVERTIME"))
        {
            attr = e->AddScaleX(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("SCALE_Y_OVERTIME"); attrnode; attrnode = attrnode.next_sibling("SCALE_Y_OVERTIME"))
        {
            attr = e->AddScaleY(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("SPIN_OVERTIME"); attrnode; attrnode = attrnode.next_sibling("SPIN_OVERTIME"))
        {
            attr = e->AddSpin(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("DIRECTION"); attrnode; attrnode = attrnode.next_sibling("DIRECTION"))
        {
            attr = e->AddDirection(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("DIRECTION_VARIATIONOT"); attrnode; attrnode = attrnode.next_sibling("DIRECTION_VARIATIONOT"))
        {
            attr = e->AddDirectionVariationOT(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("FRAMERATE_OVERTIME"); attrnode; attrnode = attrnode.next_sibling("FRAMERATE_OVERTIME"))
        {
            attr = e->AddFramerate(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("STRETCH_OVERTIME"); attrnode; attrnode = attrnode.next_sibling("STRETCH_OVERTIME"))
        {
            attr = e->AddStretch(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("RED_OVERTIME"); attrnode; attrnode = attrnode.next_sibling("RED_OVERTIME"))
        {
            attr = e->AddR(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            //LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("GREEN_OVERTIME"); attrnode; attrnode = attrnode.next_sibling("GREEN_OVERTIME"))
        {
            attr = e->AddG(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            //LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("BLUE_OVERTIME"); attrnode; attrnode = attrnode.next_sibling("BLUE_OVERTIME"))
        {
            attr = e->AddB(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            //LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("GLOBAL_VELOCITY"); attrnode; attrnode = attrnode.next_sibling("GLOBAL_VELOCITY"))
        {
            attr = e->AddGlobalVelocity(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("EMISSION_ANGLE"); attrnode; attrnode = attrnode.next_sibling("EMISSION_ANGLE"))
        {
            attr = e->AddEmissionAngle(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        for (attrnode = node.child("EMISSION_RANGE"); attrnode; attrnode = attrnode.next_sibling("EMISSION_RANGE"))
        {
            attr = e->AddEmissionRange(attrnode.attribute("FRAME").as_float(), attrnode.attribute("VALUE").as_float());
            LoadAttributeNode(attrnode, attr);
        }

        sub = node.child("EFFECT");
        if (sub)
            e->AddEffect(LoadEffect(sub, sprites, e));

        return e;
    }

    AnimImage* PugiXMLLoader::GetSpriteInList( const std::list<AnimImage*>& sprites, int index ) const
    {
        for (auto s = sprites.begin(); s != sprites.end(); ++s)
        {
            if ((*s)->GetIndex() == index)
                return *s;
        }
        return NULL;
    }

} // namespace TLFX
