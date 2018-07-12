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

#include <ignition/common/SystemPaths.hh>
#include <ignition/common/PluginLoader.hh>
#include <ignition/common/PluginPtr.hh>
#include <ignition/physics/RequestFeatures.hh>
#include <ignition/math/Rand.hh>

#include "../MockFrameSemantics.hh"


using ignition::physics::FrameData;
using ignition::physics::FrameID;
using ignition::physics::RelativeFrameData;
using ignition::physics::Pose;
using ignition::physics::Vector;
using ignition::physics::LinearVector;
using ignition::physics::AngularVector;

using ignition::math::Rand;

#define IGN_MOCK_PLUGIN_PATH \
  DETAIL_IGN_MOCK_PLUGIN_PATH

/////////////////////////////////////////////////
ignition::common::PluginPtr LoadMockFrameSemanticsPlugin(
    const std::string &_suffix)
{
  ignition::common::SystemPaths sp;
  sp.AddPluginPaths(IGN_MOCK_PLUGIN_PATH);
  const std::string path = sp.FindSharedLibrary("MockFrames");

  ignition::common::PluginLoader pl;
  auto plugins = pl.LoadLibrary(path);
  EXPECT_EQ(4u, plugins.size());

  ignition::common::PluginPtr plugin =
      pl.Instantiate("mock::MockFrameSemanticsPlugin"+_suffix);
  EXPECT_FALSE(plugin.IsEmpty());

  return plugin;
}

/////////////////////////////////////////////////
template <typename VectorType>
VectorType RandomVector(const double range)
{
  VectorType v;
  for (std::size_t i = 0; i < VectorType::RowsAtCompileTime; ++i)
    v[i] = Rand::DblUniform(-range, range);

  return v;
}

/////////////////////////////////////////////////
template <typename Scalar, std::size_t Dim>
struct Rotation
{
  /// \brief Randomize the orientation of a 3D pose
  static void Randomize(ignition::physics::Pose<Scalar, Dim> &_pose)
  {
    for (std::size_t i = 0; i < 3; ++i)
    {
      Vector<Scalar, Dim> axis = Vector<Scalar, Dim>::Zero();
      axis[i] = 1.0;
      _pose.rotate(Eigen::AngleAxis<Scalar>(
                     static_cast<Scalar>(Rand::DblUniform(0, 2*M_PI)), axis));
    }
  }

  static bool Equal(
      const Eigen::Matrix<Scalar, Dim, Dim> &_R1,
      const Eigen::Matrix<Scalar, Dim, Dim> &_R2,
      const double _tolerance)
  {
    Eigen::AngleAxis<Scalar> R(_R1.transpose() * _R2);
    if (std::abs(R.angle()) > _tolerance)
    {
      std::cout << "Difference in angle: " << R.angle() << std::endl;
      return false;
    }

    return true;
  }

  static AngularVector<Scalar, Dim> Apply(
      const Eigen::Matrix<Scalar, Dim, Dim> &_R,
      const AngularVector<Scalar, Dim> &_input)
  {
    // In 3D simulation, this is a normal multiplication
    return _R * _input;
  }
};

/////////////////////////////////////////////////
template <typename Scalar>
struct Rotation<Scalar, 2>
{
  /// \brief Randomize the orientation of a 2D pose
  static void Randomize(ignition::physics::Pose<Scalar, 2> &_pose)
  {
    _pose.rotate(Eigen::Rotation2D<Scalar>(Rand::DblUniform(0, 2*M_PI)));
  }

  static bool Equal(
      const Eigen::Matrix<Scalar, 2, 2> &_R1,
      const Eigen::Matrix<Scalar, 2, 2> &_R2,
      const double _tolerance)
  {
    // Choose the largest of either 1.0 or the size of the larger angle.
    const double scale =
        std::max(
          static_cast<Scalar>(1.0),
          std::max(
            Eigen::Rotation2D<Scalar>(_R1).angle(),
            Eigen::Rotation2D<Scalar>(_R2).angle()));

    const Eigen::Rotation2D<Scalar> R(_R1.transpose() * _R2);
    if (std::abs(R.angle()/scale) > _tolerance)
    {
      std::cout << "Scaled difference in angle: "
                << R.angle()/scale << " | Difference: " << R.angle()
                << " | Scale: " << scale
                << " | (Tolerance: " << _tolerance << ")" << std::endl;
      return false;
    }

    return true;
  }

  static AngularVector<Scalar, 2> Apply(
      const Eigen::Matrix<Scalar, 2, 2> &,
      const AngularVector<Scalar, 2> &_input)
  {
    // Angular vectors cannot be rotated in 2D simulations, so we just pass back
    // the value that was given.
    return _input;
  }
};

/////////////////////////////////////////////////
template <typename Scalar, std::size_t Dim>
FrameData<Scalar, Dim> RandomFrameData()
{
  using LinearVector = ::LinearVector<Scalar, Dim>;
  using AngularVector = ::AngularVector<Scalar, Dim>;

  FrameData<Scalar, Dim> data;
  data.pose.translation() = RandomVector<LinearVector>(100.0);
  Rotation<Scalar, Dim>::Randomize(data.pose);
  data.linearVelocity = RandomVector<LinearVector>(10.0);
  data.angularVelocity = RandomVector<AngularVector>(10.0);
  data.linearAcceleration = RandomVector<LinearVector>(1.0);
  data.angularAcceleration = RandomVector<AngularVector>(1.0);

  return data;
}

/////////////////////////////////////////////////
template <typename Scalar, int Dim>
bool Equal(const Vector<Scalar, Dim> &_vec1,
           const Vector<Scalar, Dim> &_vec2,
           const double _tolerance,
           const std::string &_label = "")
{
  // Choose the largest of either 1.0 or the length of the longer vector.
  const double scale = std::max(static_cast<Scalar>(1.0),
                                std::max(_vec1.norm(), _vec2.norm()));
  const double diff = (_vec1 - _vec2).norm();
  if (diff/scale <= _tolerance)
    return true;

  std::cout << "Scaled difference in vectors of " << _label << ": "
            << diff/scale << " | Difference: " << diff
            << " | Scale: " << scale
            << " | (Tolerance: " << _tolerance << ")" << std::endl;

  return false;
}

/////////////////////////////////////////////////
template <typename Scalar, int Dim>
bool Equal(const Pose<Scalar, Dim> &_T1,
           const Pose<Scalar, Dim> &_T2,
           const double _tolerance)
{
  if (!Equal(Vector<Scalar, Dim>(_T1.translation()),
             Vector<Scalar, Dim>(_T2.translation()),
             _tolerance, "position"))
    return false;

  if (!Rotation<Scalar, Dim>::Equal(
        _T1.linear(), _T2.linear(), _tolerance))
    return false;

  return true;
}

/////////////////////////////////////////////////
template <typename Scalar, std::size_t Dim>
bool Equal(const FrameData<Scalar, Dim> &_data1,
           const FrameData<Scalar, Dim> &_data2,
           const double _tolerance)
{
  if (!Equal(_data1.pose, _data2.pose, _tolerance))
    return false;

  if (!Equal(_data1.linearVelocity, _data2.linearVelocity,
             _tolerance, "linear velocity"))
    return false;

  if (!Equal(_data1.angularVelocity, _data2.angularVelocity,
             _tolerance, "angular velocity"))
    return false;

  if (!Equal(_data1.linearAcceleration, _data2.linearAcceleration,
             _tolerance, "linear acceleration"))
    return false;

  if (!Equal(_data1.angularAcceleration,
             _data2.angularAcceleration,
             _tolerance))
    return false;

  return true;
}

/////////////////////////////////////////////////
template <typename PolicyT>
void TestRelativeFrames(const double _tolerance, const std::string &_suffix)
{
  using Scalar = typename PolicyT::Scalar;
  constexpr std::size_t Dim = PolicyT::Dim;

  // Instantiate an engine that provides Frame Semantics.
  auto fs =
      ignition::physics::RequestFeatures<PolicyT, mock::MockFrameSemanticsList>
        ::From(LoadMockFrameSemanticsPlugin(_suffix));

  using FrameData = ::FrameData<Scalar, Dim>;
  using RelativeFrameData = ::RelativeFrameData<Scalar, Dim>;

  // Note: The World Frame is often designated by the letter O

  // Create Frame A
  const FrameData T_A = RandomFrameData<Scalar, Dim>();
  const FrameID A = fs->CreateLink("A", T_A)->GetFrameID();

  // Create Frame B
  const FrameData T_B = RandomFrameData<Scalar, Dim>();
  const FrameID B = fs->CreateLink("B", T_B)->GetFrameID();

  RelativeFrameData B_T_B = RelativeFrameData(B);
  EXPECT_TRUE(Equal(T_B, fs->Resolve(B_T_B, FrameID::World()), _tolerance));

  // Create a frame relative to A which is equivalent to B
  const RelativeFrameData A_T_B =
      RelativeFrameData(A, fs->GetLink("B")->FrameDataRelativeTo(A));

  // When A_T_B is expressed with respect to the world, it should be equivalent
  // to Frame B
  EXPECT_TRUE(Equal(T_B, fs->Resolve(A_T_B, FrameID::World()), _tolerance));

  const RelativeFrameData O_T_B = RelativeFrameData(FrameID::World(), T_B);

  // When O_T_B is expressed with respect to A, it should be equivalent to
  // A_T_B
  EXPECT_TRUE(Equal(A_T_B.RelativeToParent(),
                    fs->Resolve(O_T_B, A), _tolerance));

  // Define a new frame (C), relative to B
  const RelativeFrameData B_T_C =
      RelativeFrameData(B, RandomFrameData<Scalar, Dim>());

  // Reframe C with respect to the world
  const RelativeFrameData O_T_C = fs->Reframe(B_T_C, FrameID::World());

  // Also, compute its raw transform with respect to the world
  const FrameData T_C = fs->Resolve(B_T_C, FrameID::World());

  EXPECT_TRUE(Equal(T_C, O_T_C.RelativeToParent(), _tolerance));

  const RelativeFrameData O_T_A = RelativeFrameData(FrameID::World(), T_A);
  EXPECT_TRUE(Equal(T_C.pose,
                      O_T_A.RelativeToParent().pose
                    * A_T_B.RelativeToParent().pose
                    * B_T_C.RelativeToParent().pose, _tolerance));
}

/////////////////////////////////////////////////
TEST(FrameSemantics_TEST, RelativeFrames3d)
{
  TestRelativeFrames<ignition::physics::FeaturePolicy3d>(1e-11, "3d");
}

/////////////////////////////////////////////////
TEST(FrameSemantics_TEST, RelativeFrames2d)
{
  TestRelativeFrames<ignition::physics::FeaturePolicy2d>(1e-14, "2d");
}

/////////////////////////////////////////////////
TEST(FrameSemantics_TEST, RelativeFrames3f)
{
  TestRelativeFrames<ignition::physics::FeaturePolicy3f>(1e-3, "3f");
}

/////////////////////////////////////////////////
TEST(FrameSemantics_TEST, RelativeFrames2f)
{
  TestRelativeFrames<ignition::physics::FeaturePolicy2f>(1e-3, "2f");
}

/////////////////////////////////////////////////
template <typename PolicyT>
void TestFrameID(const double _tolerance, const std::string &_suffix)
{
  using Scalar = typename PolicyT::Scalar;
  constexpr std::size_t Dim = PolicyT::Dim;

  // Instantiate an engine that provides Frame Semantics.
  auto fs =
      ignition::physics::RequestFeatures<PolicyT, mock::MockFrameSemanticsList>
        ::From(LoadMockFrameSemanticsPlugin(_suffix));

  using FrameData = ::FrameData<Scalar, Dim>;
  using RelativeFrameData = ::RelativeFrameData<Scalar, Dim>;

  using Link = ignition::physics::Link<PolicyT, mock::MockFrameSemanticsList>;
  using LinkPtr = std::unique_ptr<Link>;

  using Joint = ignition::physics::Joint<PolicyT, mock::MockFrameSemanticsList>;
  using ConstJointPtr = std::unique_ptr<const Joint>;

  // We test FrameID in this unit test, because the FrameSemantics interface is
  // needed in order to produce FrameIDs.
  const FrameID world = FrameID::World();

  // The world FrameID is always considered to be "reference counted", because
  // it must always be treated as a valid ID.
  EXPECT_TRUE(world.IsReferenceCounted());

  const FrameData dataA = RandomFrameData<Scalar, Dim>();
  const LinkPtr linkA = fs->CreateLink("A", dataA);

  EXPECT_TRUE(Equal(dataA, linkA->FrameDataRelativeTo(world), _tolerance));

  const FrameID A = linkA->GetFrameID();
  EXPECT_TRUE(A.IsReferenceCounted());
  EXPECT_EQ(A, fs->GetLink("A")->GetFrameID());
  EXPECT_EQ(A, linkA->GetFrameID());

  // This is the implicit conversion operator which can implicitly turn a
  // FrameSemantics::Object reference into a FrameID.
  const FrameID otherA = *linkA;
  EXPECT_EQ(otherA, A);

  const FrameData dataJ1 = RandomFrameData<Scalar, Dim>();
  const ConstJointPtr joint1 = fs->CreateJoint("B", dataJ1);
  EXPECT_NE(nullptr, joint1);

  const FrameID J1 = joint1->GetFrameID();
  EXPECT_FALSE(J1.IsReferenceCounted());
  EXPECT_EQ(J1, fs->GetJoint("B")->GetFrameID());
  EXPECT_EQ(J1, joint1->GetFrameID());

  // Create relative frame data for J1 with respect to the world frame
  const RelativeFrameData O_T_J1(FrameID::World(), dataJ1);
  // Create a version which is with respect to frame A
  const RelativeFrameData A_T_J1 = fs->Reframe(O_T_J1, A);

  // When we dereference linkA, the implicit conversion operator should be able
  // to automatically convert it to a FrameID that can be used by the Frame
  // Semantics API.
  EXPECT_TRUE(Equal(A_T_J1.RelativeToParent(),
                    fs->Resolve(O_T_J1, *linkA), _tolerance));

  const RelativeFrameData J1_T_J1 = fs->Reframe(A_T_J1, *joint1);
  EXPECT_TRUE(Equal(J1_T_J1.RelativeToParent(),
                    fs->Resolve(O_T_J1, J1), _tolerance));
}

/////////////////////////////////////////////////
TEST(FrameSemantics_TEST, FrameID3d)
{
  TestFrameID<ignition::physics::FeaturePolicy3d>(1e-11, "3d");
}

/////////////////////////////////////////////////
TEST(FrameSemantics_TEST, FrameID2d)
{
  TestFrameID<ignition::physics::FeaturePolicy2d>(1e-12, "2d");
}

/////////////////////////////////////////////////
TEST(FrameSemantics_TEST, FrameID3f)
{
  TestFrameID<ignition::physics::FeaturePolicy3f>(1e-2, "3f");
}

/////////////////////////////////////////////////
TEST(FrameSemantics_TEST, FrameID2f)
{
  TestFrameID<ignition::physics::FeaturePolicy2f>(1e-4, "2f");
}

/////////////////////////////////////////////////
template <typename PolicyT>
void TestFramedQuantities(const double _tolerance, const std::string &_suffix)
{
  using Scalar = typename PolicyT::Scalar;
  constexpr std::size_t Dim = PolicyT::Dim;

  // Instantiate an engine that provides Frame Semantics.
  auto fs =
      ignition::physics::RequestFeatures<PolicyT, mock::MockFrameSemanticsList>
        ::From(LoadMockFrameSemanticsPlugin(_suffix));

  using RelativeFrameData = ::RelativeFrameData<Scalar, Dim>;
  using LinearVector = ::LinearVector<Scalar, Dim>;
  using AngularVector = ::AngularVector<Scalar, Dim>;
  using Rotation = ::Rotation<Scalar, Dim>;
  using FramedPosition = ignition::physics::FramedPosition<Scalar, Dim>;
  using FramedForce = ignition::physics::FramedForce<Scalar, Dim>;
  using FramedTorque = ignition::physics::FramedTorque<Scalar, Dim>;

  const FrameID World = FrameID::World();

  // Create a transform from the world to Frame A
  const RelativeFrameData O_T_A(World, RandomFrameData<Scalar, Dim>());
  // Instantiate Frame A
  const FrameID A = *fs->CreateLink("A", fs->Resolve(O_T_A, World));

  // Create a transform from Frame A to Frame B
  const RelativeFrameData A_T_B(A, RandomFrameData<Scalar, Dim>());
  // Instantiate Frame B using A_T_B. Note that CreateLink(~) expects to receive
  // the link's transform with respect to the world, so we use Resolve(~) before
  // passing along the FrameData
  const FrameID B = *fs->CreateLink("B", fs->Resolve(A_T_B, World));

  // Create a transform from Frame B to Frame C
  const RelativeFrameData B_T_C(B, RandomFrameData<Scalar, Dim>());
  // Instantiate Frame C using B_T_C
  const FrameID C = *fs->CreateLink("C", fs->Resolve(B_T_C, World));

  // Create a transform from Frame A to Frame D
  const RelativeFrameData A_T_D(A, RandomFrameData<Scalar, Dim>());
  // Instantiate Frame D using A_T_D
  const FrameID D = *fs->CreateLink("D", fs->Resolve(A_T_D, World));

  // Create point "1" in Frame C
  const FramedPosition C_p1(C, RandomVector<LinearVector>(10.0));
  EXPECT_TRUE(Equal(C_p1.RelativeToParent(), fs->Resolve(C_p1, C), _tolerance));

  const LinearVector C_p1_inCoordsOfWorld =
        O_T_A.RelativeToParent().pose.linear()
      * A_T_B.RelativeToParent().pose.linear()
      * B_T_C.RelativeToParent().pose.linear()
      *  C_p1.RelativeToParent();
  EXPECT_TRUE(Equal(C_p1_inCoordsOfWorld,
                    fs->Resolve(C_p1, C, World), _tolerance));

  const LinearVector C_p1_inCoordsOfD =
      A_T_D.RelativeToParent().pose.linear().transpose()
    * A_T_B.RelativeToParent().pose.linear()
    * B_T_C.RelativeToParent().pose.linear()
    *  C_p1.RelativeToParent();
  EXPECT_TRUE(Equal(C_p1_inCoordsOfD, fs->Resolve(C_p1, C, D), _tolerance));

  const LinearVector O_p1 =
        O_T_A.RelativeToParent().pose
      * A_T_B.RelativeToParent().pose
      * B_T_C.RelativeToParent().pose
      *  C_p1.RelativeToParent();
  EXPECT_TRUE(Equal(O_p1, fs->Resolve(C_p1, World), _tolerance));

  const LinearVector O_p1_inCoordsOfC =
        B_T_C.RelativeToParent().pose.linear().transpose()
      * A_T_B.RelativeToParent().pose.linear().transpose()
      * O_T_A.RelativeToParent().pose.linear().transpose()
      *  O_p1;
  EXPECT_TRUE(Equal(O_p1_inCoordsOfC, fs->Resolve(C_p1, World, C), _tolerance));

  const LinearVector O_p1_inCoordsOfD =
        A_T_D.RelativeToParent().pose.linear().transpose()
      * O_T_A.RelativeToParent().pose.linear().transpose()
      *   O_p1;
  EXPECT_TRUE(Equal(O_p1_inCoordsOfD, fs->Resolve(C_p1, World, D), _tolerance));

  const LinearVector D_p1 =
        A_T_D.RelativeToParent().pose.inverse()
      * A_T_B.RelativeToParent().pose
      * B_T_C.RelativeToParent().pose
      *  C_p1.RelativeToParent();
  EXPECT_TRUE(Equal(D_p1, fs->Resolve(C_p1, D), _tolerance));

  const LinearVector D_p1_inCoordsOfWorld =
        O_T_A.RelativeToParent().pose.linear()
      * A_T_D.RelativeToParent().pose.linear()
      *  D_p1;
  EXPECT_TRUE(Equal(D_p1_inCoordsOfWorld,
                    fs->Resolve(C_p1, D, World), _tolerance));

  const LinearVector D_p1_inCoordsOfC =
        B_T_C.RelativeToParent().pose.linear().transpose()
      * A_T_B.RelativeToParent().pose.linear().transpose()
      * A_T_D.RelativeToParent().pose.linear()
      *  D_p1;
  EXPECT_TRUE(Equal(D_p1_inCoordsOfC, fs->Resolve(C_p1, D, C), _tolerance));

  // Create point "2" in Frame D
  const FramedPosition D_p2(D, RandomVector<LinearVector>(10.0));
  EXPECT_TRUE(Equal(D_p2.RelativeToParent(), fs->Resolve(D_p2, D), _tolerance));

  const LinearVector O_p2 =
        O_T_A.RelativeToParent().pose
      * A_T_D.RelativeToParent().pose
      *  D_p2.RelativeToParent();
  EXPECT_TRUE(Equal(O_p2, fs->Resolve(D_p2, World), _tolerance));

  const LinearVector C_p2 =
        B_T_C.RelativeToParent().pose.inverse()
      * A_T_B.RelativeToParent().pose.inverse()
      * A_T_D.RelativeToParent().pose
      *  D_p2.RelativeToParent();
  EXPECT_TRUE(Equal(C_p2, fs->Resolve(D_p2, C), _tolerance));

  // Create point "3" in the World Frame
  const FramedPosition O_p3(World, RandomVector<LinearVector>(10.0));
  EXPECT_TRUE(Equal(O_p3.RelativeToParent(),
                    fs->Resolve(O_p3, World), _tolerance));

  const LinearVector O_p3_inCoordsOfC =
        B_T_C.RelativeToParent().pose.linear().transpose()
      * A_T_B.RelativeToParent().pose.linear().transpose()
      * O_T_A.RelativeToParent().pose.linear().transpose()
      *  O_p3.RelativeToParent();
  EXPECT_TRUE(Equal(O_p3_inCoordsOfC, fs->Resolve(O_p3, World, C), _tolerance));

  const LinearVector C_p3 =
        B_T_C.RelativeToParent().pose.inverse()
      * A_T_B.RelativeToParent().pose.inverse()
      * O_T_A.RelativeToParent().pose.inverse()
      *  O_p3.RelativeToParent();
  EXPECT_TRUE(Equal(C_p3, fs->Resolve(O_p3, C), _tolerance));

  const LinearVector C_p3_inCoordsOfWorld =
        O_T_A.RelativeToParent().pose.linear()
      * A_T_B.RelativeToParent().pose.linear()
      * B_T_C.RelativeToParent().pose.linear()
      *  C_p3;
  EXPECT_TRUE(Equal(C_p3_inCoordsOfWorld,
                    fs->Resolve(O_p3, C, World), _tolerance));

  // Create force "1" in Frame C
  const FramedForce C_f1(C, RandomVector<LinearVector>(10.0));
  EXPECT_TRUE(Equal(C_f1.RelativeToParent(), fs->Resolve(C_f1, C), _tolerance));

  const LinearVector O_f1 =
        O_T_A.RelativeToParent().pose.linear()
      * A_T_B.RelativeToParent().pose.linear()
      * B_T_C.RelativeToParent().pose.linear()
      *  C_f1.RelativeToParent();
  EXPECT_TRUE(Equal(O_f1, fs->Resolve(C_f1, World), _tolerance));

  const LinearVector D_f1 =
        A_T_D.RelativeToParent().pose.linear().transpose()
      * A_T_B.RelativeToParent().pose.linear()
      * B_T_C.RelativeToParent().pose.linear()
      *  C_f1.RelativeToParent();
  EXPECT_TRUE(Equal(D_f1, fs->Resolve(C_f1, D), _tolerance));

  // Create force "2" in Frame D
  const FramedForce D_f2(D, RandomVector<LinearVector>(10.0));
  EXPECT_TRUE(Equal(D_f2.RelativeToParent(), fs->Resolve(D_f2, D), _tolerance));

  const LinearVector O_f2 =
        O_T_A.RelativeToParent().pose.linear()
      * A_T_D.RelativeToParent().pose.linear()
      *  D_f2.RelativeToParent();
  EXPECT_TRUE(Equal(O_f2, fs->Resolve(D_f2, World), _tolerance));

  const LinearVector C_f2 =
        B_T_C.RelativeToParent().pose.linear().transpose()
      * A_T_B.RelativeToParent().pose.linear().transpose()
      * A_T_D.RelativeToParent().pose.linear()
      *  D_f2.RelativeToParent();
  EXPECT_TRUE(Equal(C_f2, fs->Resolve(D_f2, C), _tolerance));

  // Create force "3" in the World Frame
  const FramedForce O_f3(World, RandomVector<LinearVector>(10.0));
  EXPECT_TRUE(Equal(O_f3.RelativeToParent(),
                    fs->Resolve(O_f3, World), _tolerance));

  const LinearVector C_f3 =
        B_T_C.RelativeToParent().pose.linear().transpose()
      * A_T_B.RelativeToParent().pose.linear().transpose()
      * O_T_A.RelativeToParent().pose.linear().transpose()
      *  O_f3.RelativeToParent();
  EXPECT_TRUE(Equal(C_f3, fs->Resolve(O_f3, C), _tolerance));

  // Create torque "1" in Frame C
  const FramedTorque C_t1(C, RandomVector<AngularVector>(10.0));
  EXPECT_TRUE(Equal(C_t1.RelativeToParent(), fs->Resolve(C_t1, C), _tolerance));

  const AngularVector O_t1 =
      Rotation::Apply(
          O_T_A.RelativeToParent().pose.linear()
        * A_T_B.RelativeToParent().pose.linear()
        * B_T_C.RelativeToParent().pose.linear(),
           C_t1.RelativeToParent());
  EXPECT_TRUE(Equal(O_t1, fs->Resolve(C_t1, World), _tolerance));

  const AngularVector O_t1_inCoordsOfC =
      Rotation::Apply(
          B_T_C.RelativeToParent().pose.linear().transpose()
        * A_T_B.RelativeToParent().pose.linear().transpose()
        * O_T_A.RelativeToParent().pose.linear().transpose(),
           O_t1);
  EXPECT_TRUE(Equal(O_t1_inCoordsOfC, fs->Resolve(C_t1, World, C), _tolerance));

  const AngularVector D_t1 =
      Rotation::Apply(
          A_T_D.RelativeToParent().pose.linear().transpose()
        * A_T_B.RelativeToParent().pose.linear()
        * B_T_C.RelativeToParent().pose.linear(),
           C_t1.RelativeToParent());
  EXPECT_TRUE(Equal(D_t1, fs->Resolve(C_t1, D), _tolerance));

  const AngularVector D_t1_inCoordsOfWorld =
      Rotation::Apply(
          O_T_A.RelativeToParent().pose.linear()
        * A_T_D.RelativeToParent().pose.linear(),
           D_t1);
  EXPECT_TRUE(Equal(D_t1_inCoordsOfWorld,
                    fs->Resolve(C_t1, D, World), _tolerance));

  const AngularVector D_t1_inCoordsOfC =
      Rotation::Apply(
          B_T_C.RelativeToParent().pose.linear().transpose()
        * A_T_B.RelativeToParent().pose.linear().transpose()
        * A_T_D.RelativeToParent().pose.linear(),
           D_t1);
  EXPECT_TRUE(Equal(D_t1_inCoordsOfC, fs->Resolve(C_t1, D, C), _tolerance));

  // Create torque "2" in Frame D
  const FramedTorque D_t2(D, RandomVector<AngularVector>(10.0));
  EXPECT_TRUE(Equal(D_f2.RelativeToParent(), fs->Resolve(D_f2, D), _tolerance));

  const AngularVector O_t2 =
      Rotation::Apply(
          O_T_A.RelativeToParent().pose.linear()
        * A_T_D.RelativeToParent().pose.linear(),
           D_t2.RelativeToParent());
  EXPECT_TRUE(Equal(O_t2, fs->Resolve(D_t2, World), _tolerance));

  const AngularVector C_t2 =
      Rotation::Apply(
          B_T_C.RelativeToParent().pose.linear().transpose()
        * A_T_B.RelativeToParent().pose.linear().transpose()
        * A_T_D.RelativeToParent().pose.linear(),
           D_t2.RelativeToParent());
  EXPECT_TRUE(Equal(C_t2, fs->Resolve(D_t2, C), _tolerance));

  // Create torque "3" in the World Frame
  const FramedTorque O_t3(World, RandomVector<AngularVector>(10.0));
  EXPECT_TRUE(Equal(O_t3.RelativeToParent(),
                    fs->Resolve(O_t3, World), _tolerance));

  const AngularVector C_t3 =
      Rotation::Apply(
          B_T_C.RelativeToParent().pose.linear().transpose()
        * A_T_B.RelativeToParent().pose.linear().transpose()
        * O_T_A.RelativeToParent().pose.linear().transpose(),
           O_t3.RelativeToParent());
  EXPECT_TRUE(Equal(C_t3, fs->Resolve(O_t3, C), _tolerance));
}

/////////////////////////////////////////////////
TEST(FrameSemantics_TEST, FramedQuantities3d)
{
  TestFramedQuantities<ignition::physics::FeaturePolicy3d>(1e-11, "3d");
}

/////////////////////////////////////////////////
TEST(FrameSemantics_TEST, FramedQuantities2d)
{
  TestFramedQuantities<ignition::physics::FeaturePolicy2d>(1e-11, "2d");
}

/////////////////////////////////////////////////
TEST(FrameSemantics_TEST, FramedQuantities3f)
{
  TestFramedQuantities<ignition::physics::FeaturePolicy3f>(1e-2, "3f");
}

/////////////////////////////////////////////////
TEST(FrameSemantics_TEST, FramedQuantities2f)
{
  TestFramedQuantities<ignition::physics::FeaturePolicy2f>(1e-4, "2f");
}

int main(int argc, char **argv)
{
  // This seed is arbitrary, but we always use the same seed value to ensure
  // that results are reproduceable between runs. You may change this number,
  // but understand that the values generated in these tests will be different
  // each time that you change it. The expected tolerances might need to be
  // adjusted if the seed number is changed.
  ignition::math::Rand::Seed(416);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
