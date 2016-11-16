#include <Tools/Logs.h>
#include "FilteringBamProcessor.h"
#include "BamController.h"
#include <Estimation/CellsDataContainer.h>
#include <Tools/ReadParameters.h>

#include <iomanip>
#include <set>

namespace Estimation
{
	namespace BamProcessing
	{
		FilteringBamProcessor::FilteringBamProcessor(const CellsDataContainer &container)
			: is_bam_open(false)
			, written_reads(0)
		{
			auto const &merge_targets = container.merge_targets();
			std::vector<bool> good_cells_mask(merge_targets.size(), false);

			for (size_t id : container.filtered_cells())
			{
				good_cells_mask[id] = true;
			}

			for (size_t base_cell_id = 0; base_cell_id < merge_targets.size(); ++base_cell_id)
			{
				size_t target_cell = merge_targets[base_cell_id];
				if (!good_cells_mask[target_cell])
					continue;

				this->merge_cbs[container.cell_barcode(base_cell_id)] = container.cell_barcode(target_cell);
			}

			L_TRACE << "Writer prepared, " << this->merge_cbs.size() << " cbs to write";
		}

		void FilteringBamProcessor::trace_state(const std::string &bam_file) const
		{
			L_TRACE << bam_file << ": " << this->total_reads() << " total reads; " << this->written_reads << std::setprecision(3)
					<< " ("<< (100.0 * this->written_reads / this->total_reads()) <<"%) written";
		}

		void FilteringBamProcessor::save_read(const std::string &cell_barcode, const std::string &chr_name,
											  const std::string &umi, const std::string &gene)
		{}

		void FilteringBamProcessor::write_alignment(BamTools::BamAlignment alignment, const std::string &gene,
													const Tools::ReadParameters &read_params)
		{
			if (gene == "")
				return;
			
			auto iter = this->merge_cbs.find(read_params.cell_barcode());
			if (iter == this->merge_cbs.end())
				return;

			this->written_reads++;
			this->save_alignment(alignment, read_params.read_name_safe(), gene, iter->second, read_params.umi_barcode());
		}

		std::string FilteringBamProcessor::get_result_bam_name(const std::string &bam_name) const
		{
			return bam_name.substr(0, bam_name.find_last_of(".")) + ".filtered.bam";
		}

		void FilteringBamProcessor::update_bam(const std::string &bam_file, const BamTools::BamReader &reader)
		{
			if (this->is_bam_open)
				return;

			BamProcessorAbstract::update_bam(bam_file, reader);
			this->is_bam_open = true;
		}
	}
}