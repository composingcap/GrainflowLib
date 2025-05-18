#include <atomic>
#include<filesystem>
#include "gfIBufferReader.h"
#include "gfParam.h"
namespace Grainflow{
    template<typename SigType>
    class buffer{
        private:
        std::atomic<bool> latch_;
        std::vector<std::vector<SigType>> data_;
        gf_buffer_info info_;

        private:
            void resize(int frames, int channels, int samplerate = 0){
                if (samplerate > 0){
                    info_.samplerate = samplerate;
                    info_.one_over_samplerate = 1.0f/samplerate;
                }
                info_.one_over_buffer_frames = 1.0f/frames;
                info_.n_channels = channels;
                info.buffer_frames = frames;
                data_.resize(chanels);
                for (auto& c : channels){ 
                    c.resize(frames);
                }

            }

            void clear(){
                for (auto& c : channels){ 
                    std::fill(c.begin(), c.end(), 0);
                }
            }

        void from_interleaved(int channels, int frames, SigType* samples){
            int total_samples = channels*frames;
            data_.resize(channels);
            for (auto& c : data_){
                c.resize(frames);
            }
            for (int i = 0; i < frames; ++i){
                const auto base = i*channels;
                for (int j = 0; j < channels; ++j){
                    data_[i][j] = samples[base+j]
                }
            }
        }

        void from_sequential(int channels, int frames, SigType* samples){
            int total_samples = channels*frames;
            data_.resize(channels);
            for (auto& c : data_){
                c.resize(frames);
            }
            for (int i = 0; i < channels; ++i){
                std::copy_n(samples+(frames*i), frames, data_[i].begin());
            }
        }
        public:
        buffer(int frames, int channels, int samplerate,  SigType* samples= nullptr, bool interleaved = false){
            info_.samplerate = samplerate;
            if (samples == nullptr){
                resize(frames, channels, samplerate);
            }
            else if (interleaved){
                from_interleaved(channels, frames, samples);
            }
            else{
                from_sequential(channels, frames, samples);
            }
        }
    };
    template<typename SigType>
    struct buffer_lock{
        friend gfBuffer;
        private:
        SigType dummy_;
        buffer<SigType>* buffer_ {nullptr};
        bool valid_ {false};
        public:
        buffer_lock(buffer<SigType>* buffer){
            buffer_ = buffer;
            if (buffer->latch_.load()){
                return;
            }
            buffer->latch_.store(true);
            valid_ = true;
        }
        ~buffer_lock()
        bool valid(){
            return valid_;
        }

        bool clear(){
            buffer_->clear();
        }

        std::vector<std::vector<SigType>>& get_samples(){
            return data_;
        }

        void get_info(gf_buffer_info& info){
            info = buffer_->info_;
        }

        void read_audio_file(std::filesystem::path audio_file_path){
            buffer_->replace(audio_file_path);
        }

        void resize(int frames, int channels){
            buffer_->resize(frames, channels);
        }

        int frame_count(){
            return buffer_->info_.buffer_frames;
        }

        int channel_count(){
            return buffer->info_.n_channels;
        }

        int samplerate(){
            return buffer_->info_.samplerate;
        }

        SigType& lookup(int frame, int channel){
            if (frame > buffer->info_.frames || channel > buffer->info_.channels){
                throw;
            }
            return buffer_->data_[channel][frame];
        }

    };
    template<typename SigType>
    class gf_buffer_reader{
        public:
		static bool update_buffer_info(buffer* buffer, const gf_io_config<>& io_config,
		                               gf_buffer_info* buffer_info)
		{
			if (buffer == nullptr){
                return dummy_;
            }
			gfBufferLock buffer_lock(buffer);
            if (!buffer_lock.valid()){
                return false;
            }

            buffer_lock.get_info(*buffer_info);
            buffer_info->sample_rate_adjustment = buffer_info->samplerate / io_config.samplerate;
            
			return true;
		}

		static bool sample_param_buffer(buffer_reference* buffer, gf_param* param, const int grain_id)
		{
			std::random_device rd;
			if (param->mode == gf_buffer_mode::normal || buffer == nullptr)
			{
				return false;
			}
			buffer_lock<> param_buf(*buffer);
			if (!param_buf.valid())
				return false;
			size_t frame = 0;
			size_t frames = param_buf.frame_count();
			if (frames <= 0) return false;
			if (param->mode == gf_buffer_mode::buffer_sequence)
			{
				frame = grain_id % frames;
			}
			else if (param->mode == gf_buffer_mode::buffer_random)
			{
				frame = (rd() % frames);
			}
			param->value = param_buf.lookup(frame, 0) + param->random * (rd() % 10000) * 0.0001 + param->offset *
				grain_id;
			return true;
		}

		static void sample_buffer(buffer* buffer, const int channel, double* __restrict samples,
		                          const double* positions, 
		                          const int size, const float lower_bound, const float upper_bound)
		{
			
		}

		static void read_buffer(buffer* buffer, int channel, double* __restrict samples, int start_sample,
			const int size)
		{
			
		}

		static void write_buffer(buffer* buffer, const int channel, const double* samples,
			const int start_position, const int size)
		{
			
		}


		static void sample_envelope(buffer* buffer, const bool use_default, const int n_envelopes,
		                            const float env2d_pos, double* __restrict samples,
		                            const double* __restrict grain_clock, const int size)
		{
			
			
			
		}
        
		static gf_i_buffer_reader<buffer<SigType>> get_gf_buffer_reader()
		{
			gf_i_buffer_reader<buffer<SigType>> _bufferReader;
			_bufferReader.sample_buffer = gf_buffer_reader<SigType>::sample_buffer;
			_bufferReader.sample_envelope = gf_buffer_reader<SigType>::sample_envelope;
			_bufferReader.update_buffer_info = gf_buffer_reader<SigType>::update_buffer_info;
			_bufferReader.sample_param_buffer = gf_buffer_reader<SigType>::sample_param_buffer;
			_bufferReader.write_buffer = gf_buffer_reader<SigType>::write_buffer;
			_bufferReader.read_buffer = gf_buffer_reader<SigType>::read_buffer;
			return _bufferReader;
		}
        
    };



}