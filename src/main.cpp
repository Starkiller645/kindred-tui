#define timegm _mkgmtime 

#include <cpr/payload.h>
#include <cpr/response.h>
#include <memory>  // for allocator, __shared_ptr_access, shared_ptr
#include <string>  // for string, basic_string
#include <vector>  // for vector
#include <ctime>
#include <algorithm>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"  // for Radiobox, Horizontal, Menu, Renderer, Tab
#include "ftxui/component/component_base.hpp"      // for ComponentBase
#include "ftxui/component/screen_interactive.hpp"  // for ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for Element, separator, hbox, operator|, border
#include "champions.hpp"

using namespace ftxui;


extern const unsigned int champions_txt_len;
extern const char champions_txt[champions_txt_len];

int date_to_timestamp(std::string datestr) {
	time_t rawtime;
	struct tm *timeinfo;
	int date;
	int day = std::stoi(datestr.substr(0, datestr.find("/")));
	int month = std::stoi(datestr.substr(datestr.find("/") + 1, datestr.size()));
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	timeinfo->tm_mon = month - 1;
	timeinfo->tm_mday = day;
	timeinfo->tm_hour = 0;
	timeinfo->tm_min = 0;
	timeinfo->tm_sec = 0;
	date = timegm(timeinfo);
	return date;
}


std::string ReplaceString(std::string subject, const std::string& search,
                          const std::string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return subject;
}

std::vector<std::string> split_string(const std::string& str, const std::string& delim) {
	std::vector<std::string> strings;

	std::string::size_type pos = 0;
	std::string::size_type prev = 0;
	while((pos = str.find(delim, prev)) != std::string::npos) {
		strings.push_back(str.substr(prev, pos - prev));
		prev = pos + 1;
	}
	return strings;
}

int main(int argc, const char* argv[]) {
	std::string champs_string(champions_txt, champions_txt_len);
	auto screen = ScreenInteractive::Fullscreen();
	std::vector<std::string> champs_list = split_string(champs_string, "\n");
	nlohmann::json champs_classes;
	nlohmann::json champions;
	for(int i = 0; i < champs_list.size(); i++) {
		champions[champs_list[i].substr(0, champs_list[i].find(":"))] = champs_list[i].substr(champs_list[i].find(":") + 1, champs_list[i].find("/") - champs_list[i].find(":") - 1);
	}
	for(int i = 0; i < champs_list.size(); i++) {
        	champs_classes[champs_list[i].substr(0, champs_list[i].find(":"))] = champs_list[i].substr(champs_list[i].find("/") + 1);
    	}

	// NEXT CLASH
	std::string next_clash_name;
	std::string next_clash_date;
	std::string next_clash_start;

	auto show_next_clash = Renderer([&] {
		auto title_text = text("Next Clash") | color(Color::BlueViolet) | bold;
		auto subtitle_text = text("Upcoming clash date, time and name") | dim;
		auto name = text(next_clash_name) | color(Color::BlueViolet);
		auto date = text(next_clash_date) | bold;
		auto time = text(next_clash_start + "  ");
		return vbox({
			title_text,
			subtitle_text,
			vbox({
			name,
			hbox({
				time,
				date	
			}),
			}) | border,
		}) | size(WIDTH, EQUAL,40);
	});
	
	auto next_clash_callback = cpr::GetCallback([&](cpr::Response res) {
				auto next_clash_json = nlohmann::json::parse(res.text);
				next_clash_name = next_clash_json["name"];
				next_clash_date = next_clash_json["date"];
				next_clash_start = next_clash_json["startTime"];
				screen.PostEvent(Event::Custom);
			}
			, cpr::Url{"https://lamb.jacobtye.dev/nextclash"});

	// EVENTS
	
	enum EventType {clash, tourney, training};
	std::vector<std::string> event_type_list = {"Clash", "Tournament", "Training"};
	auto event_num_res = cpr::Get(cpr::Url{"https://lamb.jacobtye.dev/num_events"});
	int num_events = std::stoi(event_num_res.text);

	std::string next_event_name;
	std::string next_event_date;
	std::string next_event_type;

	std::string new_event_name;
	std::string new_event_date;
	nlohmann::json json_post = {};
	int new_event_type;
	auto new_event_name_input = Input(&new_event_name, "");
	auto new_event_date_input = Input(&new_event_date, "");
	auto new_event_type_input = Dropdown(&event_type_list, &new_event_type);
	auto new_event_send = Button("Enter", [&]{
		json_post["name"] = new_event_name;
		json_post["date"] = new_event_date;
		switch(new_event_type) {
			case 0:
				json_post["type"] = "clash";
				break;
			case 1:
				json_post["type"] = "tourney";
				break;
			case 2:
				json_post["type"] = "training";
				break;
		};
		json_post["timestamp"] = date_to_timestamp(new_event_date);
		cpr::PostAsync(cpr::Url{"https://lamb.jacobtye.dev/upcoming"},
				cpr::Body{json_post.dump()},
				cpr::Header{{"Content-Type", "application/json"}});
	});
	
	auto new_event_container = Container::Vertical({
		new_event_name_input,
		new_event_date_input,
		new_event_type_input,		
		new_event_send,
	});

	auto show_events = Renderer(new_event_container, [&] {
		auto next_event_name_label = text(next_event_name) | bold | color(Color::BlueViolet);
		auto next_event_date_label = text(next_event_date) | bold;
		auto next_event_type_label = text(next_event_type) | dim;
		return vbox({
			text("Events") | color(Color::BlueViolet) | bold,
			text("Add and view team events") | dim,
			window(text("Add New"), vbox({
				hbox(text("Name: ") | bold, new_event_name_input->Render()),
				hbox(text("Date: ") | bold, new_event_date_input->Render()),
				new_event_type_input->Render(),
			})) | size(WIDTH, EQUAL, 40),
			new_event_send->Render() | size(WIDTH, EQUAL, 40),
			separator(),
			vbox({
				next_event_name_label,
				next_event_date_label,
				next_event_type_label,
			}) | border | size(WIDTH, EQUAL, 40),
		});
	});
	
	nlohmann::json events_json;
	std::string didReturn = "no";
	auto events_callback = cpr::GetCallback([&](cpr::Response res) {
				events_json = nlohmann::json::parse(res.text);
				if(events_json.size() < 1) {
					didReturn = "yes";
					return;
				} else {
					next_event_name = events_json[0]["name"];
					didReturn = events_json[0]["name"];
					next_event_date = events_json[0]["date"];
					next_event_type = events_json[0]["type"];
					screen.PostEvent(Event::Custom);
				};
				
			}
			, cpr::Url{"https://lamb.jacobtye.dev/upcoming"});


	// BANS
	std::vector<std::string> bans({"", "", "", "", ""});
	auto send_bans = [&] {
		cpr::Url bans_url{"https://lamb.jacobtye.dev/bans"};
		nlohmann::json json_bans = {};
		json_bans["top"] = bans[0];
		json_bans["jgl"] = bans[1];
		json_bans["mid"] = bans[2];
		json_bans["bot"] = bans[3];
		json_bans["sup"] = bans[4];
		auto res = cpr::PostAsync(bans_url,
				cpr::Body{json_bans.dump()},
				cpr::Header{{"Content-Type", "application/json"}}
		);
	};

	std::string ban_1_placeholder = "Top";
	std::string ban_1_tag = "";
	auto ban_1 = Input(&bans[0], &ban_1_placeholder);
	std::string ban_2_placeholder = "Jungle";
	std::string ban_2_tag = "";
	auto ban_2 = Input(&bans[1], &ban_2_placeholder);
	std::string ban_3_placeholder = "Mid";
	std::string ban_3_tag = "";
	auto ban_3 = Input(&bans[2], &ban_3_placeholder);
	std::string ban_4_placeholder = "Bot";
	std::string ban_4_tag = "";
	auto ban_4 = Input(&bans[3], &ban_4_placeholder);
	std::string ban_5_placeholder = "Support";
	std::string ban_5_tag = "";
	auto ban_5 = Input(&bans[4], &ban_5_placeholder);
	auto bans_enter = Button("Enter", send_bans);
	auto bans_container = Container::Vertical({
		ban_1,
		ban_2,
		ban_3,
		ban_4,
		ban_5,
		bans_enter,
	});
    std::string ban_tags[5] = {"", "", "", "", ""};
    
    auto ban_render_new = [&] {
        bool all_correct = true;
        for(int i = 0; i < 5; i++) {
            if(champions.find(bans[i]) != champions.end()) {
                if(champions[bans[i]] != ban_tags[i]) {
                    ban_tags[i] = champions[bans[i]];
                    screen.PostEvent(Event::Custom);
                 }
            } else {
                ban_tags[i] = "";
                all_correct = false;
                screen.PostEvent(Event::Custom);
            }
        }
        if(all_correct) {
            return text("OK ✓") | color(Color::Green) | bold;
        } else {
            return text("INVALID ❌") | color(Color::Red) | dim;
        }
    };

    auto render_ban_colors = [&] (int num) {
	if(champs_classes[bans[num]] == "ct") {
		return hbox(text(ban_tags[num]) | color(Color::Plum2), text("Controller") | dim | align_right | flex_grow);
	} else if(champs_classes[bans[num]] == "tk") {
		return hbox(text(ban_tags[num]) | color(Color::DarkOliveGreen1), text("Tank") | dim | align_right | flex_grow);
	} else if(champs_classes[bans[num]] == "mg") {
		return hbox(text(ban_tags[num]) | color(Color::MediumOrchid1), text("Mage") | dim | align_right | flex_grow);
	} else if(champs_classes[bans[num]] == "as") {
		return hbox(text(ban_tags[num]) | color(Color::IndianRed1), text("Assassin") | dim | align_right | flex_grow);
	} else if(champs_classes[bans[num]] == "sp") {
		return hbox(text(ban_tags[num]) | color(Color::Gold1), text("Specialist") | dim | align_right | flex_grow);
	} else if(champs_classes[bans[num]] == "mk") {
		return hbox(text(ban_tags[num]) | color(Color::Cyan2), text("Marksman") | dim | align_right | flex_grow);
	} else if(champs_classes[bans[num]] == "fi") {
		return hbox(text(ban_tags[num]) | color(Color::DodgerBlue1), text("Fighter") | dim | align_right | flex_grow);
	} else {
		return text("");
	}
    };

	auto show_bans = Renderer(bans_container, [&] {
		auto title_text = text("Bans") | color(Color::BlueViolet) | bold;
		auto subtitle_text = text("Enter bans for an upcoming game") | dim;

		return vbox({
			title_text,
			subtitle_text,
			separator(),
			vbox({
				vbox(hbox(text("TOP: ") | bold, ban_1->Render()), render_ban_colors(0)),
				vbox(hbox(text("JGL: ") | bold, ban_2->Render()), render_ban_colors(1)),
				vbox(hbox(text("MID: ") | bold, ban_3->Render()), render_ban_colors(2)),
				vbox(hbox(text("BOT: ") | bold, ban_4->Render()), render_ban_colors(3)),
				vbox(hbox(text("SUP: ") | bold, ban_5->Render()), render_ban_colors(4)),
			}) | border,
			bans_enter->Render(),
			hflow(ban_render_new()),
		});
			
	});
	

	// PICKS
	std::vector<std::string> picks({"", "", "", "", ""});
	auto send_picks = [&] {
		cpr::Url picks_url{"https://lamb.jacobtye.dev/picks"};
		nlohmann::json json_picks = {};
		json_picks["top"] = picks[0];
		json_picks["jgl"] = picks[1];
		json_picks["mid"] = picks[2];
		json_picks["bot"] = picks[3];
		json_picks["sup"] = picks[4];
		auto res = cpr::PostAsync(picks_url,
				cpr::Body{json_picks.dump()},
				cpr::Header{{"Content-Type", "application/json"}}
		);
	};

	std::string pick_1_placeholder = "Top";
	std::string pick_1_tag = "";
	auto pick_1 = Input(&picks[0], &pick_1_placeholder);
	std::string pick_2_placeholder = "Jungle";
	std::string pick_2_tag = "";
	auto pick_2 = Input(&picks[1], &pick_2_placeholder);
	std::string pick_3_placeholder = "Mid";
	std::string pick_3_tag = "";
	auto pick_3 = Input(&picks[2], &pick_3_placeholder);
	std::string pick_4_placeholder = "Bot";
	std::string pick_4_tag = "";
	auto pick_4 = Input(&picks[3], &pick_4_placeholder);
	std::string pick_5_placeholder = "Support";
	std::string pick_5_tag = "";
	auto pick_5 = Input(&picks[4], &pick_5_placeholder);
	auto picks_enter = Button("Enter", send_picks);
	auto picks_container = Container::Vertical({
		pick_1,
		pick_2,
		pick_3,
		pick_4,
		pick_5,
		picks_enter,
	});
    std::string pick_tags[5] = {"", "", "", "", ""};
    
    auto pick_render_new = [&] {
        bool all_correct = true;
        for(int i = 0; i < 5; i++) {
            if(champions.find(picks[i]) != champions.end()) {
                if(champions[picks[i]] != pick_tags[i]) {
                    pick_tags[i] = champions[picks[i]];
                    screen.PostEvent(Event::Custom);
                 }
            } else {
                pick_tags[i] = "";
                all_correct = false;
                screen.PostEvent(Event::Custom);
            }
        }
        if(all_correct) {
            return text("OK ✓") | color(Color::Green) | bold;
        } else {
            return text("INVALID ❌") | color(Color::Red) | dim;
        }
    };

    auto render_pick_colors = [&] (int num) {
	if(champs_classes[picks[num]] == "ct") {
		return hbox(text(pick_tags[num]) | color(Color::Plum2), text("Controller") | dim | align_right | flex_grow);
	} else if(champs_classes[picks[num]] == "tk") {
		return hbox(text(pick_tags[num]) | color(Color::DarkOliveGreen1), text("Tank") | dim | align_right | flex_grow);
	} else if(champs_classes[picks[num]] == "mg") {
		return hbox(text(pick_tags[num]) | color(Color::MediumOrchid1), text("Mage") | dim | align_right | flex_grow);
	} else if(champs_classes[picks[num]] == "as") {
		return hbox(text(pick_tags[num]) | color(Color::IndianRed1), text("Assassin") | dim | align_right | flex_grow);
	} else if(champs_classes[picks[num]] == "sp") {
		return hbox(text(pick_tags[num]) | color(Color::Gold1), text("Specialist") | dim | align_right | flex_grow);
	} else if(champs_classes[picks[num]] == "mk") {
		return hbox(text(pick_tags[num]) | color(Color::Cyan2), text("Marksman") | dim | align_right | flex_grow);
	} else if(champs_classes[picks[num]] == "fi") {
		return hbox(text(pick_tags[num]) | color(Color::DodgerBlue1), text("Fighter") | dim | align_right | flex_grow);
	} else {
		return text("");
	}
    };

	auto show_picks = Renderer(picks_container, [&] {
		auto title_text = text("Picks") | color(Color::BlueViolet) | bold;
		auto subtitle_text = text("Enter picks for an upcoming game") | dim;

		return vbox({
			title_text,
			subtitle_text,
			separator(),
			vbox({
				vbox(hbox(text("TOP: ") | bold, pick_1->Render()), render_pick_colors(0)),
				vbox(hbox(text("JGL: ") | bold, pick_2->Render()), render_pick_colors(1)),
				vbox(hbox(text("MID: ") | bold, pick_3->Render()), render_pick_colors(2)),
				vbox(hbox(text("BOT: ") | bold, pick_4->Render()), render_pick_colors(3)),
				vbox(hbox(text("SUP: ") | bold, pick_5->Render()), render_pick_colors(4)),
			}) | border,
			picks_enter->Render(),
			hflow(pick_render_new()),
		});
			
	});
	
	// UPDATE SUMMONERS
	


	
	std::string summoner_name;
	std::string summoner_position;
	std::string mains[5];
	auto name_disp = Input(&summoner_name, "Name");
	auto position_disp = Input(&summoner_position, "top, jgl, mid, bot or sup");

	std::string main_1_placeholder = "Main 1";
	std::string main_1_tag = "";
	auto main_1 = Input(&mains[0], &main_1_placeholder);
	std::string main_2_placeholder = "Main 2";
	std::string main_2_tag = "";
	auto main_2 = Input(&mains[1], &main_2_placeholder);
	std::string main_3_placeholder = "Main 3";
	std::string main_3_tag = "";
	auto main_3 = Input(&mains[2], &main_3_placeholder);
	std::string main_4_placeholder = "Main 4";
	std::string main_4_tag = "";
	auto main_4 = Input(&mains[3], &main_4_placeholder);
	std::string main_5_placeholder = "Main 5";
	std::string main_5_tag = "";
	auto main_5 = Input(&mains[4], &main_5_placeholder);

	std::string main_tags[5] = {"", "", "", "", ""};
	std::vector<std::string> positions = {"top", "jgl", "mid", "bot", "sup"};


	auto send_mains = [&] {
		std::string url = "https://lamb.jacobtye.dev/summoners/" + summoner_name;
		url = ReplaceString(url, " ", "%20");
		cpr::Url mains_url{url};
		nlohmann::json json_mains = {};
		json_mains["name"] = summoner_name;
		json_mains["position"] = summoner_position;
		std::vector<std::string> json_mains_list = {mains[0], mains[1], mains[2], mains[3], mains[4]};
		json_mains["mains"] = json_mains_list;
		auto res = cpr::PostAsync(mains_url,
				cpr::Body{json_mains.dump()},
				cpr::Header{{"Content-Type", "application/json"}}
		);
	};
	auto mains_enter = Button("Enter", send_mains);
	auto mains_container = Container::Vertical({
		main_1,
		main_2,
		main_3,
		main_4,
		main_5,
		mains_enter,
	});

   
    auto main_render_new = [&] {
        bool all_correct = true;
        for(int i = 0; i < 5; i++) {
            if(champions.find(mains[i]) != champions.end()) {
                if(champions[mains[i]] != main_tags[i]) {
                    main_tags[i] = champions[mains[i]];
                    screen.PostEvent(Event::Custom);
                 }
            } else {
                main_tags[i] = "";
                all_correct = false;
                screen.PostEvent(Event::Custom);
            }
        }
	if(std::find(positions.begin(), positions.end(), summoner_position) == positions.end()) {
		all_correct = false;
	}
        if(all_correct) {
            return vbox({text("OK ✓") | color(Color::Green) | bold, text("Check that the summoner name is correct before sending") | blink | dim});
        } else {
            return text("INVALID ❌") | color(Color::Red) | dim;
        }
    };

    auto render_main_colors = [&] (int num) {
	if(champs_classes[mains[num]] == "ct") {
		return hbox(text(main_tags[num]) | color(Color::Plum2), text("Controller") | dim | align_right | flex_grow);
	} else if(champs_classes[mains[num]] == "tk") {
		return hbox(text(main_tags[num]) | color(Color::DarkOliveGreen1), text("Tank") | dim | align_right | flex_grow);
	} else if(champs_classes[mains[num]] == "mg") {
		return hbox(text(main_tags[num]) | color(Color::MediumOrchid1), text("Mage") | dim | align_right | flex_grow);
	} else if(champs_classes[mains[num]] == "as") {
		return hbox(text(main_tags[num]) | color(Color::IndianRed1), text("Assassin") | dim | align_right | flex_grow);
	} else if(champs_classes[mains[num]] == "sp") {
		return hbox(text(main_tags[num]) | color(Color::Gold1), text("Specialist") | dim | align_right | flex_grow);
	} else if(champs_classes[mains[num]] == "mk") {
		return hbox(text(main_tags[num]) | color(Color::Cyan2), text("Marksman") | dim | align_right | flex_grow);
	} else if(champs_classes[mains[num]] == "fi") {
		return hbox(text(main_tags[num]) | color(Color::DodgerBlue1), text("Fighter") | dim | align_right | flex_grow);
	} else {
		return text("");
	}
    };

	auto mains_cont = Container::Vertical({
		name_disp,
		position_disp,
		main_1,
		main_2,
		main_3,
		main_4,
		main_5,
		mains_enter
	});

	auto show_summoners = Renderer(mains_cont, [&] {
		return vbox({
			text("Update Summoner Details") | bold | color(Color::BlueViolet),
			text("Update the position and mains of a team member") | dim,
			separator(),
			hbox({text("Summoner name: ") | bold, name_disp->Render()}),
			hbox({text("Position:      ") | bold, position_disp->Render()}),
			vbox({
				vbox(hbox(text("1: ") | bold, main_1->Render()), render_main_colors(0)),
				vbox(hbox(text("2: ") | bold, main_2->Render()), render_main_colors(1)),
				vbox(hbox(text("3: ") | bold, main_3->Render()), render_main_colors(2)),
				vbox(hbox(text("4: ") | bold, main_4->Render()), render_main_colors(3)),
				vbox(hbox(text("5: ") | bold, main_5->Render()), render_main_colors(4)),
			}) | border,
			mains_enter->Render(),
			main_render_new(),
			
		});
	});

	// LIVE GAME DATA
	
	auto show_live_game = Renderer([&] {
		return text("Live Game") | color(Color::YellowLight) | bold;
	});

	// MAIN SCREEN LOGIC

	std::vector<std::string> tabs {
		"Next Clash",
		"Events",
		"Bans",
		"Picks",
		"Summoners",
		"Live Game"
	};
	int tab_sel = 0;

	auto tab_menu = Menu(&tabs, &tab_sel);
	auto tab_content = Container::Tab({
		show_next_clash,
		show_events,
		show_bans,
		show_picks,
		show_summoners,
		show_live_game,
	},
	&tab_sel);

	auto main_content = Container::Horizontal({
		tab_menu,
		tab_content,
	});

	auto main_render = Renderer(main_content, [&] {
		return hbox({
			tab_menu->Render(),
			separator(),
			tab_content->Render() | flex_grow,
		}) | border;
	});


	screen.Loop(main_render);
}
