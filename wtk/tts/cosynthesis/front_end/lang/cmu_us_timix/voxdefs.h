#define VOXNAME cmu_us_timix
#define REGISTER_VOX register_cmu_us_timix
#define UNREGISTER_VOX unregister_cmu_us_timix
#define VOXHUMAN "Zeynep"
#define VOXGENDER "female"
#define VOXVERSION 0.6

cst_voice *register_cmu_us_timix(const char *voxdir) ;
void unregister_cmu_us_timix(cst_voice *v);