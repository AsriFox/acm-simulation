#include "modules.hpp"

int main(int argc, char** argv) {
    std::cout << "Using AFF3CT " << tools::version() << std::endl;

    params p(argc, argv);
    utils u;

    #pragma omp parallel
    {
        #pragma omp single
        {
            const size_t n_threads = (size_t)omp_get_num_threads();
            u.monitors.resize(n_threads);
            u.m_list.resize(n_threads);
        }
        modules m = u.init_modules(p);

        #pragma omp barrier
        #pragma omp single
        {
            u.init(*p.terminal);
            u.terminal->legend();
        }

        m.codec->set_noise(*u.noise);
        u.noise->record_callback_update(
            [&m]() { m.codec->notify_noise_update(); }
        );
        auto sigma = m.bind_sockets();

        for (auto ebn0 = p.ebn0_min; ebn0 < p.ebn0_max; ebn0 += p.ebn0_step) {
            const auto esn0 = tools::ebn0_to_esn0(ebn0, p.R, p.modem->bps);
            std::fill(sigma.begin(), sigma.end(), tools::esn0_to_sigma(esn0, p.modem->cpm_upf));
            #pragma omp single
            {
                u.noise->set_values(sigma[0], ebn0, esn0);
                u.terminal->start_temp_report();
            }
            m.run_sim_chain(u);
            #pragma omp barrier
            #pragma omp single
            {
                u.monitor_red->reduce();
                u.terminal->final_report();
                u.monitor_red->reset();
                u.terminal->reset();
            }
            if (u.terminal->is_over()) break;
        }

        #pragma omp single
        {
            std::cout << "#" << std::endl;
            tools::Stats::show(m.list, true);
            std::cout << "# End of the simulation" << std::endl;
        }
    }
    return 0;
}
