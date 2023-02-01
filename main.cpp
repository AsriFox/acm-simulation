// #include "modules.hpp"

// int main(int argc, char** argv) {
//     std::cout << "Using AFF3CT " << tools::version() << std::endl;

//     params p(argc, argv);
//     utils u;

//     #pragma omp parallel
//     {
//         #pragma omp single
//         {
//             const size_t n_threads = (size_t)omp_get_num_threads();
//             u.monitors.resize(n_threads);
//             u.m_list.resize(n_threads);
//         }
//         modules m = u.init_modules(p);

//         #pragma omp barrier
//         #pragma omp single
//         {
//             u.init(*p.terminal);
//             u.terminal->legend();
//         }

//         m.codec->set_noise(*u.noise);
//         u.noise->record_callback_update(
//             [&m]() { m.codec->notify_noise_update(); }
//         );
//         auto sigma = m.bind_sockets();

//         for (auto ebn0 = p.ebn0_min; ebn0 < p.ebn0_max; ebn0 += p.ebn0_step) {
//             const auto esn0 = tools::ebn0_to_esn0(ebn0, p.R, p.modem->bps);
//             std::fill(sigma.begin(), sigma.end(), tools::esn0_to_sigma(esn0, p.modem->cpm_upf));
//             #pragma omp single
//             {
//                 u.noise->set_values(sigma[0], ebn0, esn0);
//                 u.terminal->start_temp_report();
//             }
//             m.run_sim_chain(u);
//             #pragma omp barrier
//             #pragma omp single
//             {
//                 u.monitor_red->reduce();
//                 u.terminal->final_report();
//                 m.monitor->reset();
//                 u.terminal->reset();
//             }
//             if (u.terminal->is_over()) break;
//         }

//         #pragma omp single
//         {
//             std::cout << "#" << std::endl;
//             tools::Stats::show(m.list, true);
//             std::cout << "# End of the simulation" << std::endl;
//         }
//     }
//     return 0;
// }

#include <functional>
#include <exception>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <memory>
#include <vector>
#include <string>

#include <aff3ct.hpp>
using namespace aff3ct;

#ifdef _OPENMP
#include <omp.h>
#else
inline int omp_get_thread_num () { return 0; }
inline int omp_get_num_threads() { return 1; }
#endif

struct params
{
	float ebn0_min  = 2.00f; // minimum SNR value
	float ebn0_max  = 4.01f; // maximum SNR value
	float ebn0_step = 0.10f; // SNR step
	float R;                 // code rate (R=K/N)

	std::unique_ptr<factory::Source      > source;
	std::unique_ptr<factory::Codec_LDPC  > codec;
	std::unique_ptr<factory::Modem       > modem;
	std::unique_ptr<factory::Channel     > channel;
	std::unique_ptr<factory::Monitor_BFER> monitor;
	std::unique_ptr<factory::Terminal    > terminal;
};
void init_params(int argc, char** argv, params &p);

namespace aff3ct { namespace tools {
using Monitor_BFER_reduction = Monitor_reduction<module::Monitor_BFER<>>;
} }

struct utils
{
	            std::unique_ptr<tools ::Sigma<>               >  noise;       // a sigma noise type
	std::vector<std::unique_ptr<tools ::Reporter              >> reporters;   // list of reporters displayed in the terminal
	            std::unique_ptr<tools ::Terminal              >  terminal;    // manage the output text in the terminal
	std::vector<std::unique_ptr<module::Monitor_BFER<>        >> monitors;    // list of the monitors from all the threads
	            std::unique_ptr<tools ::Monitor_BFER_reduction>  monitor_red; // main monitor object that reduce all the thread monitors

	std::vector<std::vector<const module::Module*>> modules;       // lists of the allocated modules
	std::vector<std::vector<const module::Module*>> modules_stats; // list of the allocated modules reorganized for the statistics
};
void init_utils(const params &p, utils &u);

struct modules
{
	std::unique_ptr<module::Source<>>       source;
	std::unique_ptr<tools ::Codec_SIHO<>>   codec;
	std::unique_ptr<module::Modem<>>        modem;
	std::unique_ptr<module::Channel<>>      channel;
	                module::Monitor_BFER<>* monitor;
	                module::Encoder<>*      encoder;
	                module::Decoder_SIHO<>* decoder;
	std::vector<const module::Module*>      list; // list of module pointers declared in this structure
};
void init_modules_and_utils(const params &p, modules &m, utils &u);

int main(int argc, char** argv)
{
	// get the AFF3CT version
	const std::string v = "v" + std::to_string(tools::version_major()) + "." +
	                            std::to_string(tools::version_minor()) + "." +
	                            std::to_string(tools::version_release());

	std::cout << "#----------------------------------------------------------"      << std::endl;
	std::cout << "# This is a basic program using the AFF3CT library (" << v << ")" << std::endl;
	std::cout << "# Feel free to improve it as you want to fit your needs."         << std::endl;
	std::cout << "#----------------------------------------------------------"      << std::endl;
	std::cout << "#"                                                                << std::endl;

	params p; init_params(argc, argv, p); // create and initialize the parameters from the command line with factories
	utils u; // create an 'utils' structure

#pragma omp parallel
{
#pragma omp single
{
	// get the number of available threads from OpenMP
	const size_t n_threads = (size_t)omp_get_num_threads();
	u.monitors.resize(n_threads);
	u.modules .resize(n_threads);
}
	modules m; init_modules_and_utils(p, m, u); // create and initialize the modules and initialize a part of the utils

#pragma omp barrier
#pragma omp single
{
	init_utils(p, u); // finalize the utils initialization

	// display the legend in the terminal
	u.terminal->legend();
}

	// set the noise and register modules to "noise changed" callback
	m.codec->set_noise(*u.noise); u.noise->record_callback_update([&m](){ m.codec->notify_noise_update(); });

	// sockets binding (connect the sockets of the tasks = fill the input sockets with the output sockets)
	using namespace module;
	(*m.encoder)[enc::sck::encode      ::U_K ].bind((*m.source )[src::sck::generate   ::U_K ]);
	(*m.modem  )[mdm::sck::modulate    ::X_N1].bind((*m.encoder)[enc::sck::encode     ::X_N ]);
	(*m.channel)[chn::sck::add_noise   ::X_N ].bind((*m.modem  )[mdm::sck::modulate   ::X_N2]);
	(*m.modem  )[mdm::sck::demodulate  ::Y_N1].bind((*m.channel)[chn::sck::add_noise  ::Y_N ]);
	(*m.decoder)[dec::sck::decode_siho ::Y_N ].bind((*m.modem  )[mdm::sck::demodulate ::Y_N2]);
	(*m.monitor)[mnt::sck::check_errors::U   ].bind((*m.source )[src::sck::generate   ::U_K ]);
	(*m.monitor)[mnt::sck::check_errors::V   ].bind((*m.decoder)[dec::sck::decode_siho::V_K ]);

	std::vector<float> sigma(1);
	(*m.channel)[chn::sck::add_noise ::CP].bind(sigma);
	(*m.modem  )[mdm::sck::demodulate::CP].bind(sigma);

	// loop over the various SNRs
	for (auto ebn0 = p.ebn0_min; ebn0 < p.ebn0_max; ebn0 += p.ebn0_step)
	{
		// compute the current sigma for the channel noise
		const auto esn0 = tools::ebn0_to_esn0 (ebn0, p.R, p.modem->bps);
		std::fill(sigma.begin(), sigma.end(), tools::esn0_to_sigma(esn0, p.modem->cpm_upf));

#pragma omp single
{
		u.noise->set_values(sigma[0], ebn0, esn0);

		// display the performance (BER and FER) in real time (in a separate thread)
		u.terminal->start_temp_report();
}

		// run the simulation chain
		while (!u.monitor_red->is_done() && !u.terminal->is_interrupt())
		{
			(*m.source )[src::tsk::generate    ].exec();
			(*m.encoder)[enc::tsk::encode      ].exec();
			(*m.modem  )[mdm::tsk::modulate    ].exec();
			(*m.channel)[chn::tsk::add_noise   ].exec();
			(*m.modem  )[mdm::tsk::demodulate  ].exec();
			(*m.decoder)[dec::tsk::decode_siho ].exec();
			(*m.monitor)[mnt::tsk::check_errors].exec();
		}

// need to wait all the threads here before to reset the 'monitors' and 'terminal' states
#pragma omp barrier
#pragma omp single
{
		// final reduction
		u.monitor_red->reduce();

		// display the performance (BER and FER) in the terminal
		u.terminal->final_report();

		// reset the monitor and the terminal for the next SNR
		u.monitor_red->reset();
		u.terminal->reset();
}
		// if user pressed Ctrl+c twice, exit the SNRs loop
		if (u.terminal->is_over()) break;
	}

#pragma omp single
{
	// display the statistics of the tasks (if enabled)
	std::cout << "#" << std::endl;
	tools::Stats::show(u.modules_stats, true);
	std::cout << "# End of the simulation" << std::endl;
}
}
	return 0;
}

void init_params(int argc, char** argv, params &p)
{
	p.source   = std::unique_ptr<factory::Source      >(new factory::Source      ());
	p.codec    = std::unique_ptr<factory::Codec_LDPC  >(new factory::Codec_LDPC  ());
	p.modem    = std::unique_ptr<factory::Modem       >(new factory::Modem       ());
	p.channel  = std::unique_ptr<factory::Channel     >(new factory::Channel     ());
	p.monitor  = std::unique_ptr<factory::Monitor_BFER>(new factory::Monitor_BFER());
	p.terminal = std::unique_ptr<factory::Terminal    >(new factory::Terminal    ());

	std::vector<factory::Factory*> params_list = { p.source .get(), p.codec  .get(), p.modem   .get(),
	                                               p.channel.get(), p.monitor.get(), p.terminal.get() };

	// parse the command for the given parameters and fill them
	tools::Command_parser cp(argc, argv, params_list, true);
	if (cp.parsing_failed())
	{
		cp.print_help    ();
		cp.print_warnings();
		cp.print_errors  ();
		std::exit(1);
	}

	std::cout << "# Simulation parameters: " << std::endl;
	tools::Header::print_parameters(params_list); // display the headers (= print the AFF3CT parameters on the screen)
	std::cout << "#" << std::endl;
	cp.print_warnings();

	p.R = (float)p.codec->enc->K / (float)p.codec->enc->N_cw; // compute the code rate
}

void init_modules_and_utils(const params &p, modules &m, utils &u)
{
	// get the thread id from OpenMP
	const int tid = omp_get_thread_num();

	// set different seeds for different threads when the module use a PRNG
	p.source->seed += tid;
	p.channel->seed += tid;

	m.source        = std::unique_ptr<module::Source      <>>(p.source ->build());
	m.codec         = std::unique_ptr<tools ::Codec_SIHO  <>>(p.codec  ->build());
	m.modem         = std::unique_ptr<module::Modem       <>>(p.modem  ->build());
	m.channel       = std::unique_ptr<module::Channel     <>>(p.channel->build());
	u.monitors[tid] = std::unique_ptr<module::Monitor_BFER<>>(p.monitor->build());
	m.monitor       = u.monitors[tid].get();
	m.encoder       = &m.codec->get_encoder();
	m.decoder       = &m.codec->get_decoder_siho();

	m.list = { m.source.get(), m.modem.get(), m.channel.get(), m.monitor, m.encoder, m.decoder };
	u.modules[tid] = m.list;

	// configuration of the module tasks
	for (auto& mod : m.list)
		for (auto& tsk : mod->tasks)
		{
			tsk->set_autoalloc  (true ); // enable the automatic allocation of the data in the tasks
			tsk->set_debug      (false); // disable the debug mode
			tsk->set_debug_limit(16   ); // display only the 16 first bits if the debug mode is enabled
			tsk->set_stats      (true ); // enable the statistics

			// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
			if (!tsk->is_debug() && !tsk->is_stats())
				tsk->set_fast(true);
		}
}

void init_utils(const params &p, utils &u)
{
	// allocate a common monitor module to reduce all the monitors
	u.monitor_red = std::unique_ptr<tools::Monitor_BFER_reduction>(new tools::Monitor_BFER_reduction(u.monitors));
	u.monitor_red->set_reduce_frequency(std::chrono::milliseconds(500));
	// create a sigma noise type
	u.noise = std::unique_ptr<tools::Sigma<>>(new tools::Sigma<>());
	// report the noise values (Es/N0 and Eb/N0)
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_noise<>(*u.noise)));
	// report the bit/frame error rates
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_BFER<>(*u.monitor_red)));
	// report the simulation throughputs
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_throughput<>(*u.monitor_red)));
	// create a terminal that will display the collected data from the reporters
	u.terminal = std::unique_ptr<tools::Terminal>(p.terminal->build(u.reporters));

	u.modules_stats.resize(u.modules[0].size());
	for (size_t m = 0; m < u.modules[0].size(); m++)
		for (size_t t = 0; t < u.modules.size(); t++)
			u.modules_stats[m].push_back(u.modules[t][m]);
}