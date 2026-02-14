#include "connection/connection_services.h"
#include "tests/test_harness.h"

using namespace scratchrobin;
using namespace scratchrobin::connection;

namespace {

void ExpectReject(const std::string& code, const std::function<void()>& fn) {
    try {
        fn();
    } catch (const RejectError& ex) {
        scratchrobin::tests::AssertEq(ex.payload().code, code, "unexpected reject");
        return;
    }
    throw std::runtime_error("expected reject not thrown");
}

runtime::ConnectionProfile BuildProfile(std::string backend, runtime::ConnectionMode mode = runtime::ConnectionMode::Network) {
    runtime::ConnectionProfile p;
    p.name = "p1";
    p.backend = std::move(backend);
    p.mode = mode;
    p.host = "127.0.0.1";
    p.database = "db1";
    p.username = "u1";
    p.credential_id = "cred";
    return p;
}

}  // namespace

int main() {
    std::vector<std::pair<std::string, scratchrobin::tests::TestFn>> tests;

    tests.push_back({"integration/connection_backend_selection", [] {
                        BackendAdapterService svc;
                        auto s = svc.Connect(BuildProfile("pg"));
                        scratchrobin::tests::AssertEq(s.backend_name, "postgresql", "backend mismatch");
                        scratchrobin::tests::AssertTrue(s.port == 5432, "default port mismatch");
                        scratchrobin::tests::AssertTrue(s.connected, "session should be connected");
                    }});

    tests.push_back({"integration/connection_capability_gate", [] {
                        BackendAdapterService svc;
                        svc.Connect(BuildProfile("mock"));
                        ExpectReject("SRB1-R-4101", [&] { svc.ExecutePrepared("select 1", {}); });
                    }});

    tests.push_back({"integration/connection_copy_prepared_status", [] {
                        BackendAdapterService svc;
                        svc.Connect(BuildProfile("firebird"));
                        auto copy = svc.ExecuteCopy("COPY t TO STDOUT", "stdin", "stdout", true, true);
                        scratchrobin::tests::AssertEq(copy, "copy-ok", "copy mismatch");
                        auto prep = svc.ExecutePrepared("select ? from rdb$database", {"1"});
                        scratchrobin::tests::AssertTrue(prep.find("prepared-ok") == 0U, "prepare mismatch");
                        auto status = svc.FetchStatus(1, 0);
                        scratchrobin::tests::AssertTrue(status.find("running_queries") != std::string::npos, "status mismatch");
                    }});

    tests.push_back({"integration/connection_notifications", [] {
                        BackendAdapterService svc;
                        svc.Connect(BuildProfile("scratchbird"));
                        svc.Subscribe("chan", "payload");
                        auto evt = svc.FetchNotification();
                        scratchrobin::tests::AssertTrue(evt.has_value(), "expected notification");
                        scratchrobin::tests::AssertEq(evt->first, "chan", "channel mismatch");
                        svc.Unsubscribe("chan");

                        BackendAdapterService svc2;
                        svc2.Connect(BuildProfile("mysql"));
                        ExpectReject("SRB1-R-4204", [&] { svc2.Subscribe("chan", "payload"); });
                    }});

    tests.push_back({"integration/connection_cancel_active", [] {
                        BackendAdapterService svc;
                        svc.Connect(BuildProfile("scratchbird"));
                        svc.MarkActiveQuery(true);
                        svc.CancelActiveQuery();
                        ExpectReject("SRB1-R-4206", [&] { svc.CancelActiveQuery(); });
                    }});

    tests.push_back({"integration/connection_enterprise_flow", [] {
                        BackendAdapterService svc;

                        beta1b::EnterpriseConnectionProfile p;
                        p.profile_id = "prod";
                        p.username = "svc";
                        p.transport = beta1b::TransportContract{"ssh_jump_chain", "required", 1000};
                        p.ssh = beta1b::SshContract{"db.internal", 5432, "svc", "keypair", "cred_ssh"};
                        p.jump_hosts.push_back(beta1b::JumpHost{"bastion", 22, "jump", "agent", ""});
                        p.identity = beta1b::IdentityContract{"oidc", "idp", {"openid"}};
                        p.secret_provider = beta1b::SecretProviderContract{"vault", "kv/data/x"};

                        const auto fp = svc.ConnectEnterprise(
                            p,
                            std::nullopt,
                            [](const beta1b::SecretProviderContract&) { return std::optional<std::string>("secret"); },
                            [](const std::string&) { return std::optional<std::string>("credential"); },
                            [](const std::string&, const std::string&) { return true; },
                            [](const std::string&, const std::string&) { return true; });

                        scratchrobin::tests::AssertEq(fp.profile_id, "prod", "profile mismatch");
                        scratchrobin::tests::AssertEq(fp.transport_mode, "ssh_jump_chain", "transport mismatch");
                    }});

    return scratchrobin::tests::RunTests(tests);
}

