/**
 * @file CentroidalMPC.h
 * @authors Giulio Romualdi
 * @copyright 2023 Istituto Italiano di Tecnologia (IIT). This software may be modified and
 * distributed under the terms of the BSD-3-Clause license.
 */

#ifndef BIPEDAL_LOCOMOTION_REDUCE_MODEL_CONTROLLERS_CENTROIDAL_MPC_H
#define BIPEDAL_LOCOMOTION_REDUCE_MODEL_CONTROLLERS_CENTROIDAL_MPC_H

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <BipedalLocomotion/Contacts/Contact.h>
#include <BipedalLocomotion/Contacts/ContactPhaseList.h>
#include <BipedalLocomotion/ParametersHandler/IParametersHandler.h>
#include <BipedalLocomotion/System/Source.h>

namespace BipedalLocomotion
{
namespace ReducedModelControllers
{

/**
 * CentroidalMPCOutput contains the output of the CentroidalMPC class
 */
struct CentroidalMPCOutput
{
    std::map<std::string, Contacts::DiscreteGeometryContact> contacts;
    std::map<std::string, Contacts::PlannedContact> nextPlannedContact;
    std::vector<Eigen::Vector3d> comTrajectory;
};

/**
 * CentroidalMPC implements a Non-Linear Model Predictive Controller for humanoid robot locomotion
 * with online step adjustment capabilities. The proposed controller considers the Centroidal
 * Dynamics of the system to compute the desired contact forces and torques and contact locations.
 * Let us assume the presence of a high-level contact planner that generates only the contact
 * location and timings, the CentroidalMPC objective is to implement a control law that generates
 * feasible contact wrenches and locations while considering the Centroidal dynamics of the floating
 * base system, and a nominal set of contact positions and timings. The control problem is
 * formulated using the Model Predictive Control (MPC) framework.
 * @note This class implements the work presented in G. Romualdi, S. Dafarra, G. L'Erario, I.
 * Sorrentino, S. Traversaro and D. Pucci, "Online Non-linear Centroidal MPC for Humanoid Robot
 * Locomotion with Step Adjustment," 2022 International Conference on Robotics and Automation
 * (ICRA), Philadelphia, PA, USA, 2022, pp. 10412-10419, doi: 10.1109/ICRA46639.2022.9811670.
 */
class CentroidalMPC : public System::Source<CentroidalMPCOutput>
{
public:
    /**
     * Constructor.
     */
    CentroidalMPC();

    /**
     * Destructor.
     */
    ~CentroidalMPC();

    // clang-format off
    /**
     * Initialize the controller.
     * @param handler pointer to the parameter handler.
     * @note the following parameters are required by the class
     * |          Parameter Name         |       Type       |                                                                                          Description                                                                                         | Mandatory |
     * |:-------------------------------:|:----------------:|:--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------:|:---------:|
     * |         `sampling_time`         |     `double`     |                                                                                   Sampling time of the MPC.                                                                                  |    Yes    |
     * |          `time_horizon`         |     `double`     |                                            The time horizon of the MPC. The number of knots will be given by `floor(time_horizon / sampling_time)`                                           |    Yes    |
     * |   `number_of_maximum_contacts`  |       `int`      |                            Integer representing the maximum number of contacts that can be established. For a bipedal is in general 2 (the feet) for a quadruped 4.                           |    Yes    |
     * |           `com_weight`          | `vector<double>` |                       Weight of the CoM in the cost function. The Vector must contain three elements that will correspond to the weight in the x y and z coordinates.                        |    Yes    |
     * |    `contact_position_weight`    |     `double`     |                                           Weight related to the contact position regularization provided by the CentroidalMPC::setContactPhaseList                                           |    Yes    |
     * |  `force_rate_of_change_weight`  | `vector<double>` |                               Weight associated to the rate of change of the contact forces. The higher the weight, the more the contact forces will be smooth.                              |    Yes    |
     * |    `angular_momentum_weight`    |     `double`     |                                 Weight associated to the angular momentum. The higher the weight, the more the angular momentum will follow the desired one.                                 |    Yes    |
     * | `contact_force_symmetry_weight` |     `double`     |                 Weight associated to the symmetry of the contact forces. The higher the weight, the more the contact forces associated to the same contact will be symmetric                 |    Yes    |
     * |         `linear_solver`         |     `string`     |                             Linear solver used by ipopt. Please check https://coin-or.github.io/Ipopt/#PREREQUISITES for the available solvers (default `mumps`).                            |    Yes    |
     * |        `ipopt_tolerance`        |     `double`     |                        Determines the convergence tolerance for the algorithm (default value is \f$10^{-8}\f$ (https://coin-or.github.io/Ipopt/OPTIONS.html#OPT_tol).                        |     No    |
     * |      `ipopt_max_iteration`      |       `int`      |                                                            The maximum number of iterations of ipopt (The default value is 3000).                                                            |     No    |
     * |        `solver_verbosity`       |       `int`      |                                                Verbosity of the solver. The higher the value, the higher the verbosity (Default value is `0`)                                                |     No    |
     * |     `is_warm_start_enabled`     |      `bool`      |                                 True if the user wants to warm start the CoM, angular momentum, and contact location with the nominal value (Default `false`)                                |     No    |
     * |         `is_cse_enabled`        |      `bool`      | True if the Common subexpression elimination casadi option is enabled. This option is supported only by casadi 3.6.0 https://github.com/casadi/casadi/releases/tag/3.6.3  (Default `false` ) |     No    |
     *
     * Moreover for each contact \f$i\f$ where \f$ 0 \le i \le \f$ `number_of_maximum_contacts-1` it is required to define a group `CONTACT_<i>` that contains the following parameters
     * |       Parameter Name       |        Type      |                                                          Description                                                             | Mandatory |
     * |:--------------------------:|:----------------:|:--------------------------------------------------------------------------------------------------------------------------------:|:---------:|
     * |      `contact_name`        |     `string`     |                                                 Name associated to the contact.                                                  |    Yes    |
     * | `bounding_box_upper_limit` | `vector<double>` | Upper limit of the bounding box where the adjusted contact must belong to. The limits are expressed in the contact local frame.  |    Yes    |
     * | `bounding_box_lower_limit` | `vector<double>` | Lower limit of the bounding box where the adjusted contact must belong to. The limits are expressed in the contact local frame.  |    Yes    |
     * |    `number_of_corners`     |      `int`       |                                             Number of corners associated to the foot.                                            |    Yes    |
     * |        `corner_<j>`        | `vector<double>` |                 Position of the corner expressed in the foot frame. I must be from 0 to number_of_corners - 1.                   |    Yes    |
     * @return true in case of success/false otherwise.
     */
    bool initialize(std::weak_ptr<const ParametersHandler::IParametersHandler> handler) final;
    // clang-format on

    /**
     * Set the contact phase list considered by the controller as nominal contact location.
     * @param contactPhaseList contact phase.
     * @return True in case of success, false otherwise.
     * @note This function needs to be called before advance.
     */
    bool setContactPhaseList(const Contacts::ContactPhaseList& contactPhaseList);

    /**
     * Set the state of the centroidal dynamics.
     * @param com position of the CoM expressed in the inertial frame.
     * @param dcom velocity of the CoM expressed in a frame centered in the CoM and oriented as the
     * inertial frame.
     * @param angularMomentum centroidal angular momentum.
     * @return True in case of success, false otherwise.
     * @note This function needs to be called before advance.
     * @note The external wrench is assumed to be zero.
     */
    bool setState(Eigen::Ref<const Eigen::Vector3d> com,
                  Eigen::Ref<const Eigen::Vector3d> dcom,
                  Eigen::Ref<const Eigen::Vector3d> angularMomentum);

    /**
     * Set the state of the centroidal dynamics.
     * @param com position of the CoM expressed in the inertial frame.
     * @param dcom velocity of the CoM expressed in a frame centered in the CoM and oriented as the
     * inertial frame.
     * @param angularMomentum centroidal angular momentum.
     * @param externalWrench optional parameter used to represent an external wrench applied to the
     * robot CoM.
     * @return True in case of success, false otherwise.
     * @note This function needs to be called before advance.
     */
    bool setState(Eigen::Ref<const Eigen::Vector3d> com,
                  Eigen::Ref<const Eigen::Vector3d> dcom,
                  Eigen::Ref<const Eigen::Vector3d> angularMomentum,
                  const Math::Wrenchd& externalWrench);

    /**
     * Set the reference trajectories for the CoM and the centroidal angular momentum.
     * @param com desired trajectory of the CoM. The rows contain the x, y and z coordinates while
     * the columns the position at each time instant.
     * @param angularMomentum centroidal angular momentum. The rows contain the x, y and z
     * coordinates while the columns the trajectory at each time instant.
     * @return True in case of success, false otherwise.
     * @note In case the warmstart has been enabled in the initialization, then the CoM and the
     * angular momentum will be used to warmstart the problem.
     * @warning The CoM and the angular momentum trajectory is assumed to be sampled at the
     * controller sampling period
     */
    bool setReferenceTrajectory(const std::vector<Eigen::Vector3d>& com,
                                const std::vector<Eigen::Vector3d>& angularMomentum);

    /**
     * Get the output of the controller
     * @return a const reference of the output of the controller.
     */
    const CentroidalMPCOutput& getOutput() const final;

    /**
     * Determines the validity of the object retrieved with getOutput()
     * @return True if the object is valid, false otherwise.
     */
    bool isOutputValid() const final;

    /**
     * Perform one control cycle.
     * @return True if the advance is successfull.
     */
    bool advance() final;

private:
    /**
     * Private implementation
     */
    struct Impl;

    std::unique_ptr<Impl> m_pimpl; /**< Pointer to private implementation */
};
} // namespace ReducedModelControllers
} // namespace BipedalLocomotion

#endif // BIPEDAL_LOCOMOTION_REDUCE_MODEL_CONTROLLERS_CENTROIDAL_MPC_H
