#include "modules.hpp"

int main(int argc, char** argv) {
    std::cout << "Using AFF3CT " << tools::version() << std::endl;

    params  p(argc, argv);
    modules m(p);
    utils   u(p, *m.monitor);
    
    u.terminal->legend();

    m.codec->set_noise(*u.noise);
    u.noise->record_callback_update(
        [&m]() { m.codec->notify_noise_update(); }
    );
    auto sigma = m.bind_sockets();

    for (auto ebn0 = p.ebn0_min; ebn0 < p.ebn0_max; ebn0 += p.ebn0_step) {
        const auto esn0 = tools::ebn0_to_esn0(ebn0, p.R, p.modem->bps);
        std::fill(sigma.begin(), sigma.end(), tools::esn0_to_sigma(esn0, p.modem->cpm_upf));
        u.noise->set_values(sigma[0], ebn0, esn0);
        u.terminal->start_temp_report();
        m.run_sim_chain(u);
        u.terminal->final_report();
        m.monitor->reset();
        u.terminal->reset();
        if (u.terminal->is_over()) break;
    }

    std::cout << "#" << std::endl;
    tools::Stats::show(m.list, true);
    std::cout << "# End of the simulation" << std::endl;
    return 0;
}
