#include <sipstack/ParameterTypes.hxx>

using namespace Vocal2;

Data ParameterTypes::ParameterNames[MAX_PARAMETER] = {0};

ParameterType<ParameterTypes::transport> Vocal2::p_transport;
ParameterType<ParameterTypes::user> Vocal2::p_user;
ParameterType<ParameterTypes::method> Vocal2::p_method;
ParameterType<ParameterTypes::ttl> Vocal2::p_ttl;
ParameterType<ParameterTypes::maddr> Vocal2::p_maddr;
ParameterType<ParameterTypes::lr> Vocal2::p_lr;
ParameterType<ParameterTypes::q> Vocal2::p_q;
ParameterType<ParameterTypes::purpose> Vocal2::p_purpose;
ParameterType<ParameterTypes::expires> Vocal2::p_expires;
ParameterType<ParameterTypes::handling> Vocal2::p_handling;
ParameterType<ParameterTypes::tag> Vocal2::p_tag;
ParameterType<ParameterTypes::duration> Vocal2::p_duration;
ParameterType<ParameterTypes::branch> Vocal2::p_branch;
ParameterType<ParameterTypes::received> Vocal2::p_received;
ParameterType<ParameterTypes::comp> Vocal2::p_com;
ParameterType<ParameterTypes::rport> Vocal2::p_rport;
