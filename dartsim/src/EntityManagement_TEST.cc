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

#include <gtest/gtest.h>

#include <gz/plugin/Loader.hh>

#include <gz/common/geospatial/Dem.hh>
#include <gz/common/geospatial/ImageHeightmap.hh>
#include <gz/common/MeshManager.hh>
#include <gz/common/Filesystem.hh>

#include <gz/math/eigen3/Conversions.hh>

#include <gz/physics/Joint.hh>
#include <gz/physics/RequestEngine.hh>
#include <gz/physics/RevoluteJoint.hh>

#include "EntityManagementFeatures.hh"
#include "JointFeatures.hh"
#include "KinematicsFeatures.hh"
#include "ShapeFeatures.hh"

struct TestFeatureList : gz::physics::FeatureList<
    gz::physics::dartsim::EntityManagementFeatureList,
    gz::physics::dartsim::JointFeatureList,
    gz::physics::dartsim::KinematicsFeatureList,
    gz::physics::dartsim::ShapeFeatureList
> { };

TEST(EntityManagement_TEST, ConstructEmptyWorld)
{
  gz::plugin::Loader loader;
  loader.LoadLib(dartsim_plugin_LIB);

  gz::plugin::PluginPtr dartsim =
      loader.Instantiate("gz::physics::dartsim::Plugin");

  auto engine =
      gz::physics::RequestEngine3d<TestFeatureList>::From(dartsim);
  ASSERT_NE(nullptr, engine);

  auto world = engine->ConstructEmptyWorld("empty world");
  ASSERT_NE(nullptr, world);
  EXPECT_EQ("empty world", world->GetName());
  EXPECT_EQ(engine, world->GetEngine());

  auto model = world->ConstructEmptyModel("empty model");
  ASSERT_NE(nullptr, model);
  EXPECT_EQ("empty model", model->GetName());
  EXPECT_EQ(world, model->GetWorld());
  EXPECT_NE(model, world->ConstructEmptyModel("dummy"));

  auto nestedModel = model->ConstructEmptyNestedModel("empty nested model");
  ASSERT_NE(nullptr, nestedModel);
  EXPECT_EQ("empty nested model", nestedModel->GetName());
  EXPECT_EQ(1u, model->GetNestedModelCount());
  EXPECT_EQ(world, nestedModel->GetWorld());
  EXPECT_EQ(0u, model->GetIndex());
  EXPECT_EQ(nestedModel, model->GetNestedModel(0));
  EXPECT_EQ(nestedModel, model->GetNestedModel("empty nested model"));
  EXPECT_NE(nestedModel, nestedModel->ConstructEmptyNestedModel("dummy"));
  // This should remain 1 since we're adding a nested model in `nestedModel` not
  // in `model`.
  EXPECT_EQ(1u, model->GetNestedModelCount());
  EXPECT_EQ(1u, nestedModel->GetNestedModelCount());

  auto link = model->ConstructEmptyLink("empty link");
  ASSERT_NE(nullptr, link);
  EXPECT_EQ("empty link", link->GetName());
  EXPECT_EQ(model, link->GetModel());
  EXPECT_NE(link, model->ConstructEmptyLink("dummy"));
  EXPECT_EQ(0u, link->GetIndex());
  EXPECT_EQ(model, link->GetModel());

  auto joint = link->AttachRevoluteJoint(nullptr);
  EXPECT_NEAR((Eigen::Vector3d::UnitX() - joint->GetAxis()).norm(), 0.0, 1e-6);
  EXPECT_DOUBLE_EQ(0.0, joint->GetPosition(0));

  joint->SetAxis(Eigen::Vector3d::UnitZ());
  EXPECT_NEAR((Eigen::Vector3d::UnitZ() - joint->GetAxis()).norm(), 0.0, 1e-6);

  auto child = model->ConstructEmptyLink("child link");
  EXPECT_EQ(2u, child->GetIndex());
  EXPECT_EQ(model, child->GetModel());

  const std::string boxName = "box";
  const Eigen::Vector3d boxSize(0.1, 0.2, 0.3);
  auto box = link->AttachBoxShape(boxName, boxSize);
  EXPECT_EQ(boxName, box->GetName());
  EXPECT_NEAR((boxSize - box->GetSize()).norm(), 0.0, 1e-6);

  EXPECT_EQ(1u, link->GetShapeCount());
  auto boxCopy = link->GetShape(0u);
  EXPECT_EQ(box, boxCopy);


  auto prismatic = child->AttachPrismaticJoint(
        link, "prismatic", Eigen::Vector3d::UnitZ());
  const double zPos = 2.5;
  const double zVel = 9.1;
  const double zAcc = 10.2;
  prismatic->SetPosition(0, zPos);
  prismatic->SetVelocity(0, zVel);
  prismatic->SetAcceleration(0, zAcc);

  const gz::physics::FrameData3d childData =
      child->FrameDataRelativeToWorld();

  const Eigen::Vector3d childPosition = childData.pose.translation();
  EXPECT_DOUBLE_EQ(0.0, childPosition.x());
  EXPECT_DOUBLE_EQ(0.0, childPosition.y());
  EXPECT_DOUBLE_EQ(zPos, childPosition.z());

  const Eigen::Vector3d childVelocity = childData.linearVelocity;
  EXPECT_DOUBLE_EQ(0.0, childVelocity.x());
  EXPECT_DOUBLE_EQ(0.0, childVelocity.y());
  EXPECT_DOUBLE_EQ(zVel, childVelocity.z());

  const Eigen::Vector3d childAcceleration = childData.linearAcceleration;
  EXPECT_DOUBLE_EQ(0.0, childAcceleration.x());
  EXPECT_DOUBLE_EQ(0.0, childAcceleration.y());
  EXPECT_DOUBLE_EQ(zAcc, childAcceleration.z());

  const double yPos = 11.5;
  Eigen::Isometry3d childSpherePose = Eigen::Isometry3d::Identity();
  childSpherePose.translate(Eigen::Vector3d(0.0, yPos, 0.0));
  auto sphere = child->AttachSphereShape("child sphere", 1.0, childSpherePose);

  const gz::physics::FrameData3d sphereData =
      sphere->FrameDataRelativeToWorld();

  const Eigen::Vector3d spherePosition = sphereData.pose.translation();
  EXPECT_DOUBLE_EQ(0.0, spherePosition.x());
  EXPECT_DOUBLE_EQ(yPos, spherePosition.y());
  EXPECT_DOUBLE_EQ(zPos, spherePosition.z());

  const Eigen::Vector3d sphereVelocity = sphereData.linearVelocity;
  EXPECT_DOUBLE_EQ(0.0, sphereVelocity.x());
  EXPECT_DOUBLE_EQ(0.0, sphereVelocity.y());
  EXPECT_DOUBLE_EQ(zVel, sphereVelocity.z());

  const Eigen::Vector3d sphereAcceleration = sphereData.linearAcceleration;
  EXPECT_DOUBLE_EQ(0.0, sphereAcceleration.x());
  EXPECT_DOUBLE_EQ(0.0, sphereAcceleration.y());
  EXPECT_DOUBLE_EQ(zAcc, sphereAcceleration.z());

  const gz::physics::FrameData3d relativeSphereData =
      sphere->FrameDataRelativeTo(*child);
  const Eigen::Vector3d relativeSpherePosition =
      relativeSphereData.pose.translation();
  EXPECT_DOUBLE_EQ(0.0, relativeSpherePosition.x());
  EXPECT_DOUBLE_EQ(yPos, relativeSpherePosition.y());
  EXPECT_DOUBLE_EQ(0.0, relativeSpherePosition.z());

  auto meshLink = model->ConstructEmptyLink("mesh_link");
  meshLink->AttachFixedJoint(child, "fixed");

  const std::string meshFilename = gz::common::joinPaths(
      GZ_PHYSICS_RESOURCE_DIR, "chassis.dae");
  auto &meshManager = *gz::common::MeshManager::Instance();
  auto *mesh = meshManager.Load(meshFilename);

  auto meshShape = meshLink->AttachMeshShape("chassis", *mesh);
  const auto originalMeshSize = mesh->Max() - mesh->Min();
  const auto meshShapeSize = meshShape->GetSize();

  // Note: dartsim uses assimp for storing mesh data, and assimp by default uses
  // single floating point precision (instead of double precision), so we can't
  // expect these values to be exact.
  for (std::size_t i = 0; i < 3; ++i)
    EXPECT_NEAR(originalMeshSize[i], meshShapeSize[i], 1e-6);

  EXPECT_NEAR(meshShapeSize[0], 0.5106, 1e-4);
  EXPECT_NEAR(meshShapeSize[1], 0.3831, 1e-4);
  EXPECT_NEAR(meshShapeSize[2], 0.1956, 1e-4);

  const gz::math::Pose3d pose(0, 0, 0.2, 0, 0, 0);
  const gz::math::Vector3d scale(0.5, 1.0, 0.25);
  auto meshShapeScaled = meshLink->AttachMeshShape("small_chassis", *mesh,
                          gz::math::eigen3::convert(pose),
                          gz::math::eigen3::convert(scale));
  const auto meshShapeScaledSize = meshShapeScaled->GetSize();

  // Note: dartsim uses assimp for storing mesh data, and assimp by default uses
  // single floating point precision (instead of double precision), so we can't
  // expect these values to be exact.
  for (std::size_t i = 0; i < 3; ++i)
    EXPECT_NEAR(originalMeshSize[i] * scale[i], meshShapeScaledSize[i], 1e-6);

  EXPECT_NEAR(meshShapeScaledSize[0], 0.2553, 1e-4);
  EXPECT_NEAR(meshShapeScaledSize[1], 0.3831, 1e-4);
  EXPECT_NEAR(meshShapeScaledSize[2], 0.0489, 1e-4);

  // image heightmap
  auto heightmapLink = model->ConstructEmptyLink("heightmap_link");
  heightmapLink->AttachFixedJoint(child, "heightmap_joint");

  auto heightmapFilename = gz::common::joinPaths(
      GZ_PHYSICS_RESOURCE_DIR, "heightmap_bowl.png");
  gz::common::ImageHeightmap data;
  EXPECT_EQ(0, data.Load(heightmapFilename));

  const gz::math::Vector3d size({129, 129, 10});
  auto heightmapShape = heightmapLink->AttachHeightmapShape("heightmap", data,
      gz::math::eigen3::convert(pose),
      gz::math::eigen3::convert(size));

  EXPECT_NEAR(size.X(), heightmapShape->GetSize()[0], 1e-6);
  EXPECT_NEAR(size.Y(), heightmapShape->GetSize()[1], 1e-6);
  EXPECT_NEAR(size.Z(), heightmapShape->GetSize()[2], 1e-6);

  auto heightmapShapeGeneric = heightmapLink->GetShape("heightmap");
  ASSERT_NE(nullptr, heightmapShapeGeneric);
  EXPECT_EQ(nullptr, heightmapShapeGeneric->CastToBoxShape());
  auto heightmapShapeRecast = heightmapShapeGeneric->CastToHeightmapShape();
  ASSERT_NE(nullptr, heightmapShapeRecast);
  EXPECT_NEAR(size.X(), heightmapShapeRecast->GetSize()[0], 1e-6);
  EXPECT_NEAR(size.Y(), heightmapShapeRecast->GetSize()[1], 1e-6);
  EXPECT_NEAR(size.Z(), heightmapShapeRecast->GetSize()[2], 1e-6);

  //  dem heightmap
  auto demLink = model->ConstructEmptyLink("dem_link");
  demLink->AttachFixedJoint(child, "dem_joint");

  auto demFilename = gz::common::joinPaths(
      GZ_PHYSICS_RESOURCE_DIR, "volcano.tif");
  gz::common::Dem dem;
  EXPECT_EQ(0, dem.Load(demFilename));

  gz::math::Vector3d sizeDem;
  sizeDem.X(dem.WorldWidth());
  sizeDem.Y(dem.WorldHeight());
  sizeDem.Z(dem.MaxElevation() - dem.MinElevation());

  auto demShape = demLink->AttachHeightmapShape("dem", dem,
      gz::math::eigen3::convert(pose),
      gz::math::eigen3::convert(sizeDem));

  // there is a loss in precision with large dems since heightmaps use floats
  EXPECT_NEAR(sizeDem.X(), demShape->GetSize()[0], 1e-3);
  EXPECT_NEAR(sizeDem.Y(), demShape->GetSize()[1], 1e-3);
  EXPECT_NEAR(sizeDem.Z(), demShape->GetSize()[2], 1e-6);

  auto demShapeGeneric = demLink->GetShape("dem");
  ASSERT_NE(nullptr, demShapeGeneric);
  EXPECT_EQ(nullptr, demShapeGeneric->CastToBoxShape());
  auto demShapeRecast = demShapeGeneric->CastToHeightmapShape();
  ASSERT_NE(nullptr, demShapeRecast);
  EXPECT_NEAR(sizeDem.X(), demShapeRecast->GetSize()[0], 1e-3);
  EXPECT_NEAR(sizeDem.Y(), demShapeRecast->GetSize()[1], 1e-3);
  EXPECT_NEAR(sizeDem.Z(), demShapeRecast->GetSize()[2], 1e-6);
}

TEST(EntityManagement_TEST, RemoveEntities)
{
  gz::plugin::Loader loader;
  loader.LoadLib(dartsim_plugin_LIB);

  gz::plugin::PluginPtr dartsim =
      loader.Instantiate("gz::physics::dartsim::Plugin");

  auto engine =
      gz::physics::RequestEngine3d<TestFeatureList>::From(dartsim);
  ASSERT_NE(nullptr, engine);

  auto world = engine->ConstructEmptyWorld("empty world");
  ASSERT_NE(nullptr, world);
  auto model = world->ConstructEmptyModel("empty model");
  ASSERT_NE(nullptr, model);

  auto modelAlias = world->GetModel(0);

  model->Remove();
  EXPECT_TRUE(model->Removed());
  EXPECT_TRUE(modelAlias->Removed());
  EXPECT_EQ(nullptr, world->GetModel(0));
  EXPECT_EQ(nullptr, world->GetModel("empty model"));
  EXPECT_EQ(0ul, world->GetModelCount());

  // Calling GetName shouldn't throw
  EXPECT_EQ("empty model", model->GetName());

  auto model2 = world->ConstructEmptyModel("model2");
  ASSERT_NE(nullptr, model2);
  EXPECT_EQ(0ul, model2->GetIndex());
  world->RemoveModel(0);
  EXPECT_EQ(0ul, world->GetModelCount());

  auto parentModel = world->ConstructEmptyModel("parent model");
  ASSERT_NE(nullptr, parentModel);
  EXPECT_EQ(0u, parentModel->GetNestedModelCount());
  auto nestedModel1 =
      parentModel->ConstructEmptyNestedModel("empty nested model1");
  ASSERT_NE(nullptr, nestedModel1);
  EXPECT_EQ(1u, parentModel->GetNestedModelCount());

  EXPECT_TRUE(parentModel->RemoveNestedModel(0));
  EXPECT_EQ(0u, parentModel->GetNestedModelCount());
  EXPECT_TRUE(nestedModel1->Removed());

  auto nestedModel2 =
      parentModel->ConstructEmptyNestedModel("empty nested model2");
  ASSERT_NE(nullptr, nestedModel2);
  EXPECT_EQ(nestedModel2, parentModel->GetNestedModel(0));
  EXPECT_TRUE(parentModel->RemoveNestedModel("empty nested model2"));
  EXPECT_EQ(0u, parentModel->GetNestedModelCount());
  EXPECT_TRUE(nestedModel2->Removed());

  auto nestedModel3 =
      parentModel->ConstructEmptyNestedModel("empty nested model3");
  ASSERT_NE(nullptr, nestedModel3);
  EXPECT_EQ(nestedModel3, parentModel->GetNestedModel(0));
  EXPECT_TRUE(nestedModel3->Remove());
  EXPECT_EQ(0u, parentModel->GetNestedModelCount());
  EXPECT_TRUE(nestedModel3->Removed());

  auto nestedModel4 =
      parentModel->ConstructEmptyNestedModel("empty nested model4");
  ASSERT_NE(nullptr, nestedModel4);
  EXPECT_EQ(nestedModel4, parentModel->GetNestedModel(0));
  // Remove the parent model and check that the nested model is removed as well
  EXPECT_TRUE(parentModel->Remove());
  EXPECT_TRUE(nestedModel4->Removed());
}

TEST(EntityManagement_TEST, ModelByIndexWithNestedModels)
{
  gz::plugin::Loader loader;
  loader.LoadLib(dartsim_plugin_LIB);

  gz::plugin::PluginPtr dartsim =
      loader.Instantiate("gz::physics::dartsim::Plugin");

  auto engine =
      gz::physics::RequestEngine3d<TestFeatureList>::From(dartsim);
  ASSERT_NE(nullptr, engine);

  auto world = engine->ConstructEmptyWorld("empty world");
  ASSERT_NE(nullptr, world);
  auto model1 = world->ConstructEmptyModel("model1");
  ASSERT_NE(nullptr, model1);
  EXPECT_EQ(0ul, model1->GetIndex());

  auto parentModel = world->ConstructEmptyModel("parent model");
  ASSERT_NE(nullptr, parentModel);
  EXPECT_EQ(1ul, parentModel->GetIndex());

  auto nestedModel1 =
      parentModel->ConstructEmptyNestedModel("empty nested model1");
  ASSERT_NE(nullptr, nestedModel1);
  EXPECT_EQ(0ul, nestedModel1->GetIndex());

  auto model2 = world->ConstructEmptyModel("model2");
  ASSERT_NE(nullptr, model2);
  EXPECT_EQ(2ul, model2->GetIndex());
  EXPECT_TRUE(model2->Remove());

  auto model2Again = world->ConstructEmptyModel("model2_again");
  ASSERT_NE(nullptr, model2Again);
  EXPECT_EQ(2ul, model2Again->GetIndex());
}
