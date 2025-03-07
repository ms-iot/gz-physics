/*
 * Copyright (C) 2018 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#ifndef GZ_PHYSICS_DARTSIM_SRC_SHAPEFEATURES_HH_
#define GZ_PHYSICS_DARTSIM_SRC_SHAPEFEATURES_HH_

#include <string>

#include <gz/physics/Shape.hh>
#include <gz/physics/BoxShape.hh>
#include <gz/physics/CapsuleShape.hh>
#include <gz/physics/CylinderShape.hh>
#include <gz/physics/EllipsoidShape.hh>
#include <gz/physics/heightmap/HeightmapShape.hh>
#include <gz/physics/mesh/MeshShape.hh>
#include <gz/physics/PlaneShape.hh>
#include <gz/physics/SphereShape.hh>

#include "Base.hh"

namespace gz {
namespace physics {
namespace dartsim {

struct ShapeFeatureList : FeatureList<
  GetShapeKinematicProperties,
  SetShapeKinematicProperties,
#if DART_VERSION_AT_LEAST(6, 10, 0)
  GetShapeFrictionPyramidSlipCompliance,
  SetShapeFrictionPyramidSlipCompliance,
#endif
  GetShapeBoundingBox,

  GetBoxShapeProperties,
  // dartsim cannot yet update shape properties without reloading the model into
  // the world
//  SetBoxShapeProperties,
  AttachBoxShapeFeature,

  GetCapsuleShapeProperties,
  //  SetCapsulerShapeProperties,
  AttachCapsuleShapeFeature,

  GetCylinderShapeProperties,
//  SetCylinderShapeProperties,
  AttachCylinderShapeFeature,

  GetEllipsoidShapeProperties,
  //  SetEllipsoidShapeProperties,
  AttachEllipsoidShapeFeature,

  GetSphereShapeProperties,
//  SetSphereShapeProperties,
  AttachSphereShapeFeature,

  heightmap::GetHeightmapShapeProperties,
//  heightmap::SetHeightmapShapeProperties,
  heightmap::AttachHeightmapShapeFeature,

  mesh::GetMeshShapeProperties,
//  mesh::SetMeshShapeProperties,
  mesh::AttachMeshShapeFeature,
  GetPlaneShapeProperties,
//  SetPlaneShapeProperties,
  AttachPlaneShapeFeature
> { };

class ShapeFeatures :
    public virtual Base,
    public virtual Implements3d<ShapeFeatureList>
{
  // ----- Kinematic Properties -----
  public: Pose3d GetShapeRelativeTransform(
      const Identity &_shapeID) const override;

  public: void SetShapeRelativeTransform(
      const Identity &_shapeID, const Pose3d &_pose) override;


  // ----- Box Features -----
  public: Identity CastToBoxShape(
      const Identity &_shapeID) const override;

  public: LinearVector3d GetBoxShapeSize(
      const Identity &_boxID) const override;

  public: Identity AttachBoxShape(
      const Identity &_linkID,
      const std::string &_name,
      const LinearVector3d &_size,
      const Pose3d &_pose) override;

  // ----- Capsule Features -----
  public: Identity CastToCapsuleShape(
      const Identity &_shapeID) const override;

  public: double GetCapsuleShapeRadius(
      const Identity &_capsuleID) const override;

  public: double GetCapsuleShapeLength(
      const Identity &_capsuleID) const override;

  public: Identity AttachCapsuleShape(
      const Identity &_linkID,
      const std::string &_name,
      double _radius,
      double _length,
      const Pose3d &_pose) override;

  // ----- Cylinder Features -----
  public: Identity CastToCylinderShape(
      const Identity &_shapeID) const override;

  public: double GetCylinderShapeRadius(
      const Identity &_cylinderID) const override;

  public: double GetCylinderShapeHeight(
      const Identity &_cylinderID) const override;

  public: Identity AttachCylinderShape(
      const Identity &_linkID,
      const std::string &_name,
      double _radius,
      double _height,
      const Pose3d &_pose) override;

  // ----- Ellipsoid Features -----
  public: Identity CastToEllipsoidShape(
      const Identity &_shapeID) const override;

  public: Vector3d GetEllipsoidShapeRadii(
      const Identity &_ellipsoidID) const override;

  public: Identity AttachEllipsoidShape(
      const Identity &_linkID,
      const std::string &_name,
      const Vector3d _radii,
      const Pose3d &_pose) override;

  // ----- Sphere Features -----
  public: Identity CastToSphereShape(
      const Identity &_shapeID) const override;

  public: double GetSphereShapeRadius(
      const Identity &_sphereID) const override;

  public: Identity AttachSphereShape(
      const Identity &_linkID,
      const std::string &_name,
      double _radius,
      const Pose3d &_pose) override;


  // ----- Heightmap Features -----
  public: Identity CastToHeightmapShape(
      const Identity &_shapeID) const override;

  public: LinearVector3d GetHeightmapShapeSize(
      const Identity &_heightmapID) const override;

  public: Identity AttachHeightmapShape(
      const Identity &_linkID,
      const std::string &_name,
      const common::HeightmapData &_heightmapData,
      const Pose3d &_pose,
      const LinearVector3d &_size,
      int _subSampling) override;

  // ----- Mesh Features -----
  public: Identity CastToMeshShape(
      const Identity &_shapeID) const override;

  public: LinearVector3d GetMeshShapeSize(
      const Identity &_meshID) const override;

  public: LinearVector3d GetMeshShapeScale(
      const Identity &_meshID) const override;

  public: Identity AttachMeshShape(
      const Identity &_linkID,
      const std::string &_name,
      const gz::common::Mesh &_mesh,
      const Pose3d &_pose,
      const LinearVector3d &_scale) override;

  // ----- Boundingbox Features -----
  public: AlignedBox3d GetShapeAxisAlignedBoundingBox(
              const Identity &_shapeID) const override;

  // ----- Plane Features -----
  public: Identity CastToPlaneShape(
      const Identity &_shapeID) const override;

  public: LinearVector3d GetPlaneShapeNormal(
      const Identity &_planeID) const override;

  public: LinearVector3d GetPlaneShapePoint(
      const Identity &_planeID) const override;

  public: Identity AttachPlaneShape(
      const Identity &_linkID,
      const std::string &_name,
      const LinearVector3d &_normal,
      const LinearVector3d &_point) override;

#if DART_VERSION_AT_LEAST(6, 10, 0)
  // ----- Friction Features -----
  public: virtual double GetShapeFrictionPyramidPrimarySlipCompliance(
            const Identity &_shapeID) const override;

  public: virtual double GetShapeFrictionPyramidSecondarySlipCompliance(
            const Identity &_shapeID) const override;

  public: virtual bool SetShapeFrictionPyramidPrimarySlipCompliance(
            const Identity &_shapeID, double _value) override;

  public: virtual bool SetShapeFrictionPyramidSecondarySlipCompliance(
            const Identity &_shapeID, double _value) override;
#endif
};

}
}
}

#endif
