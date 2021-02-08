VEC3 TCompCamera::HandleCollisionZoom(const VEC3& camPos, const VEC3& targetPos, float minOffsetDist)
{
  TCompTransform* c = get<TCompTransform>();
  //Calculates the distance between the camera and the target
  float offsetDist = VEC3::Distance(targetPos, camPos);
  // The maximun distance of the raycast is the minimun distance to the target - the actual distance
  float raycastLength = offsetDist - minOffsetDist;
  if (raycastLength < 0.f)
  {
    // camera is already too near the lookat target
    return camPos;
  }

  // -front
  VEC3 camOut = (targetPos - camPos);
  camOut.Normalize();
  //Nearest camera position posible
  VEC3 nearestCamPos = targetPos - camOut * minOffsetDist;
  //Value to store the minimun distance registered by the raycasts
  float minHitFraction = raycastLength;

  //calculates the different points of the near plain of the camera to cast the rays from there.
  float WAngle = atan(tan(getFov() / 2) * getAspectRatio());
  VEC3 Far_Up_Vector = c->getUp() * getNear() * tan(getFov() / 2);
  VEC3 Far_Left_Vector = c->getLeft() * getNear() * (tan(WAngle));
  VEC3 Distance_Vector = camOut * getNear();
  std::vector<VEC3> frustumNearCorner;
  frustumNearCorner.push_back(Far_Up_Vector + Far_Left_Vector + Distance_Vector);
  frustumNearCorner.push_back(Far_Up_Vector - Far_Left_Vector + Distance_Vector);

  frustumNearCorner.push_back(-Far_Up_Vector + Far_Left_Vector + Distance_Vector);
  frustumNearCorner.push_back(-Far_Up_Vector - Far_Left_Vector + Distance_Vector);

  float hitFraction = raycastLength;

  /*PFar_Up_Vector + PFar_Left_Vector + PDistance_Vector,
      PFar_Up_Vector - PFar_Left_Vector + PDistance_Vector,
      PFar_Up_Vector + PFar_Left_Vector + PDistance_Vector,
      PFar_Up_Vector - PFar_Left_Vector + PDistance_Vector*/
  for (int i = 0; i < 4; i++)
  {
    const VEC3& corner = frustumNearCorner[i];
    //preparation for the raycast
    VEC3 offsetToCorner = corner;
    VEC3 rayStart = nearestCamPos + offsetToCorner;
    VEC3 rayEnd = corner + camPos;
    VEC3 Dir = rayEnd - rayStart;
    Dir.Normalize();
    PxRaycastBuffer hitCall;
    PxQueryFilterData fd = PxQueryFilterData();
    fd.data.word0 = CModulePhysicsSinn::FilterGroup::Scenario | CModulePhysicsSinn::FilterGroup::Obstacles;

    bool status = CEngine::get().getPhysics().gScene->raycast(VEC3_TO_PXVEC3(rayStart), VEC3_TO_PXVEC3(Dir), raycastLength, hitCall, PxHitFlags(PxHitFlag::eDEFAULT), fd);

    //If a raycast hits anything, stores the dsistance and compares it to the saved one(and changes it if the resulting distance is lower)
    if (status) {
      if (hitCall.hasAnyHits()) {
        const PxRaycastHit& overlapHit = hitCall.getAnyHit(0);
        hitFraction = overlapHit.distance;
        minHitFraction = std::min(hitFraction, minHitFraction);
      }
    }
  }

  if (minHitFraction < raycastLength)
  {
      //returns the camera position
      return nearestCamPos - camOut * (minHitFraction);

  }
  else
  {
    return camPos;
  }
}