

  void TCompColShadow::projectConvex(VEC3 origin, VEC3 up, VEC3 left, physx::PxShape* shape)
  {
    TCompTransform* c_trans = get<TCompTransform>();

    PxConvexMeshGeometry convex = shape->getGeometry().convexMesh();

    // Extracts the Vertices of the Mesh
    const PxVec3* Vertices = convex.convexMesh->getVertices();
    PxU32 nVertices = convex.convexMesh->getNbVertices();



    MAT44 e_quat = toTransform(shape->getLocalPose()).asMatrix() * c_trans->asMatrix();

    //Creates the Vector where we store the vertices than we need to filter with the convelHull
    std::vector<VEC3> VtxsToFilter;
    PxVec3 Dir;

    //creates the Vector of final Vertices so we can store the lights origin
    std::vector<PxVec3> volumeVtx;
    volumeVtx.push_back(VEC3_TO_PXVEC3(origin));

    //projects all the vertices of the convex mesh on the light plane
    for (int i = 0; i < nVertices; i++) {
      PxVec3 V = *(Vertices + i);
      VEC3 Global_V = transformPoint(PXVEC3_TO_VEC3(V), e_quat);
      V = VEC3_TO_PXVEC3(Global_V);

      //Compruba el Punto de proyeccion sobre el plano Z_far de la luz.
      Dir = V - volumeVtx[0];
      Dir.normalize();
      PxRaycastBuffer hitCall;
      PxQueryFilterData fd = PxQueryFilterData();
      fd.data.word0 = CModulePhysicsSinn::FilterGroup::Light_Limit;
      bool status = CEngine::get().getPhysics().gScene->raycast(volumeVtx[0], Dir, 10000.f, hitCall, PxHitFlags(PxHitFlag::eDEFAULT), fd);
      if (status)
      {
        if (hitCall.hasAnyHits())
        {
          //Vertice proyectado sobre el plano
          VtxsToFilter.push_back(PXVEC3_TO_VEC3(hitCall.block.position));
        }
      }
    }
    VEC3 PlaneOrigin = VtxsToFilter[0];
    std::vector<VEC2> Vtxs2D;
    //Convert to 2D Vectexes
    Vtxs2D = convertTo2D(PlaneOrigin, up, left, VtxsToFilter);

    //ConvexHull 2D
    Vtxs2D = ConvexHull(Vtxs2D);

    std::vector<VEC3> Vtxs3D;
    //Convert to 3D
    Vtxs3D = convertTo3D(PlaneOrigin, up, left, Vtxs2D);

    for (int i = 0; i < Vtxs3D.size(); i++) {
      volumeVtx.push_back(VEC3_TO_PXVEC3(Vtxs3D[i]));
    }

    //Crea el actor Shadow
    if (actor == nullptr) {
      CEngine::get().getPhysics().createShadow(*this);
    }
    //crea el convexMesh
    PxConvexMeshDesc convexDesc;
    convexDesc.points.count = volumeVtx.size();
    convexDesc.points.stride = sizeof(PxVec3);
    convexDesc.points.data = volumeVtx.data();
    convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX | PxConvexFlag::eDISABLE_MESH_VALIDATION | PxConvexFlag::eFAST_INERTIA_COMPUTATION;
    PxShape* shapeCon = CEngine::get().getPhysics().Create_Convex_Mesh(convexDesc);
    assert(shapeCon);


    TCompCollider* c_collider = get<TCompCollider>();
    if (c_collider->isControllerBox()) {
      //Grupos de filtros de physX(es una shadow Box y colisiona con shadow_Move(player y futuros pictogramas enemigos, asi como las cajas movibles)
      CEngine::get().getPhysics().setupFiltering(
        shapeCon,
        CModulePhysicsSinn::FilterGroup::Movable_Shadow_Boxes,
        CModulePhysicsSinn::FilterGroup::Shadow_Move
      );
    }
    else if (shape->getQueryFilterData().word0 == CModulePhysicsSinn::FilterGroup::Not_Collision_Up) {
        CEngine::get().getPhysics().setupFiltering(
            shapeCon,
            CModulePhysicsSinn::FilterGroup::Not_Collision_Up_shadow,
            CModulePhysicsSinn::FilterGroup::Shadow_Move
        );
    }
    else {

      //Grupos de filtros de physX(es una shadow Box y colisiona con shadow_Move(player y futuros pictogramas enemigos, asi como las cajas movibles)
      CEngine::get().getPhysics().setupFiltering(
        shapeCon,
        CModulePhysicsSinn::FilterGroup::Non_Movable_Shadow_Boxes,
        CModulePhysicsSinn::FilterGroup::Shadow_Move
      );
    }
    //Check if it's creating the shadow of  a trigger
    if (shape->getFlags().isSet(PxShapeFlag::eTRIGGER_SHAPE)) {
      shapeCon->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
      shapeCon->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
      //shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, false);
      //shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, false);
    }
    actor->attachShape(*shapeCon);
    shapeCon->release();

  }
