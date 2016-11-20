/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Commons/Ad/Ad.cpp
 * @author Karen Arutyunov
 * $Id:$
 */
#include <stdint.h>

#include <string>
#include <sstream>

#include <El/Exception.hpp>
#include <El/Net/HTTP/URL.hpp>

#include "Ad.hpp"

namespace NewsGate
{
  namespace Ad
  {
    //
    // Condition struct
    //
    std::string Condition::NONE("[none]");
    std::string Condition::ANY("[any]");
    El::Lang Condition::NONE_LANG(El::Lang::nonexistent);
    El::Lang Condition::ANY_LANG(El::Lang::nonexistent2);

    //
    // Selector::CreativeWeightStrategy struct
    // 
    Selector::CreativeWeightStrategy*
    Selector::CreativeWeightStrategy::create(StrategyType type)
      throw(Exception, El::Exception)
    {
      switch(type)
      {
      case ST_NONE: return new NoneCreativeWeightStrategy();
      case ST_PROBABILISTIC: return new ProbabilisticCreativeWeightStrategy();
      default:
        {
          std::ostringstream ostr;
          ostr << "Ad::Selector::CreativeWeightStrategy::create: "
            "unknown type " << type;
          
          throw Exception(ostr.str());
        }
      }
    }

    //
    // Selector::ProbabilisticCreativeWeightStrategy struct
    //
    void
    Selector::ProbabilisticCreativeWeightStrategy::set_weights(
      CreativeWeightArray& creative_weights)
      throw(El::Exception)
    {
      if(creative_weights.size() < 2)
      {
        return;
      }
      
      std::sort(creative_weights.begin(), creative_weights.end());

      for(CreativeWeightArray::iterator i(creative_weights.begin()),
            e(creative_weights.end()); i != e; ++i)
      {
        i->weight *= 100;
      }
      
      double min_weight = creative_weights[0].weight;
      double max_weight = creative_weights[creative_weights.size() - 1].weight;
      
      double weight_interval = weight_zones > 1 ?
        std::max((max_weight - min_weight) / weight_zones, 1.0) : UINT32_MAX;
      
      double next_group_weight = min_weight;
      double cumulative_weight = 0;
      
      CWGArray cwg_array;
      cwg_array.reserve(creative_weights.size());
      
      for(CreativeWeightArray::const_iterator i(creative_weights.begin()),
            e(creative_weights.end()); i != e; ++i)
      {
//        std::cerr << i->weight << " ";
        
        const CreativeWeight& cw = *i;
        double weight = cw.weight;
        
        if(weight >= next_group_weight)
        {
          cwg_array.push_back(CWG());
          CWG& cwg = *cwg_array.rbegin();

          cwg.cumulative_weight = cumulative_weight;
          next_group_weight = weight + weight_interval;
        }

        CWG& cwg = *cwg_array.rbegin();
        cwg.cumulative_weight += weight;
        cwg.total_weight += weight;
        cwg.creative_weights.push_back(cw);
        cumulative_weight += weight;
      }

//      std::cerr << std::endl;
//      dump(cwg_array);
      downgrade_weights(cwg_array.rbegin(), cwg_array.rend());

//      std::cerr << "--- downgrade ---\n";      
//      dump(cwg_array);

//      std::cerr << "--- selecting ---\n";

      CreativeWeightArray new_creative_weights;
      new_creative_weights.resize(creative_weights.size());

      CreativeWeightArray::reverse_iterator i(new_creative_weights.rbegin());
      CreativeWeightArray::reverse_iterator e(new_creative_weights.rend());

      CreativeWeightArray::const_reverse_iterator j(creative_weights.rbegin());
      
      for(; !cwg_array.empty(); ++i, ++j)
      {
        assert(i != e);

        CreativeWeight& cw = *i;
        
        cw = random_take(cwg_array);
        cw.weight = j->creative->weight;
        
//        std::cerr << cw.creative->weight << "->" << cw.weight << " ";
      }

      assert(i == e);

      new_creative_weights.swap(creative_weights);

//      std::cerr << std::endl;
    }

    void
    Selector::ProbabilisticCreativeWeightStrategy::dump(
      const CWGArray& cwga) const throw()
    {
      for(CWGArray::const_iterator i(cwga.begin()), e(cwga.end()); i != e; ++i)
      {
        const CWG& cwg = *i;
        
        std::cerr << "{ " << cwg.total_weight << ", " << cwg.cumulative_weight
                  << ", [ ";

        for(CreativeWeightArray::const_iterator
              i(cwg.creative_weights.begin()), e(cwg.creative_weights.end());
            i != e; ++i)
        {
          std::cerr << i->weight << " ";
        }

        std::cerr << "] }\n";
      }      
    }
    
    //
    // Selector class
    //

    Selector::Selector(uint64_t group_cap_tm,
                       uint64_t group_cap_max_cnt,
                       uint64_t counter_cap_tm,
                       uint64_t counter_cap_max_cnt,
                       Ad::SelectorUpdateNumber update_num)
      throw(El::Exception)
        : group_cap_timeout(group_cap_tm),
          group_cap_max_count(group_cap_max_cnt),
          counter_cap_timeout(counter_cap_tm),
          counter_cap_max_count(counter_cap_max_cnt),
          update_number(update_num),
          creative_weight_strategy(new NoneCreativeWeightStrategy())
    {
      add_page(PI_DESK_PAPER, SI_DESK_PAPER_FIRST, SI_DESK_PAPER_LAST);
      add_page(PI_DESK_NLINE, SI_DESK_NLINE_FIRST, SI_DESK_NLINE_LAST);
      add_page(PI_DESK_COLUMN, SI_DESK_COLUMN_FIRST, SI_DESK_COLUMN_LAST);
      add_page(PI_TAB_PAPER, SI_TAB_PAPER_FIRST, SI_TAB_PAPER_LAST);
      add_page(PI_TAB_NLINE, SI_TAB_NLINE_FIRST, SI_TAB_NLINE_LAST);
      add_page(PI_TAB_COLUMN, SI_TAB_COLUMN_FIRST, SI_TAB_COLUMN_LAST);
      add_page(PI_MOB_NLINE, SI_MOB_NLINE_FIRST, SI_MOB_NLINE_LAST);
      add_page(PI_MOB_COLUMN, SI_MOB_COLUMN_FIRST, SI_MOB_COLUMN_LAST);
      add_page(PI_DESK_MESSAGE, SI_DESK_MESSAGE_FIRST, SI_DESK_MESSAGE_LAST);
      add_page(PI_TAB_MESSAGE, SI_TAB_MESSAGE_FIRST, SI_TAB_MESSAGE_LAST);
      add_page(PI_MOB_MESSAGE, SI_MOB_MESSAGE_FIRST, SI_MOB_MESSAGE_LAST);
    }

    void
    Selector::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << (uint32_t)1 << update_number << group_cap_timeout
           << group_cap_max_count << counter_cap_timeout
           << counter_cap_max_count;
      
      bstr.write_map(conditions);
      bstr.write_map(capped_groups_min_times);

      bstr << (uint32_t)pages.size();
        
      for(PageMap::const_iterator i(pages.begin()), e(pages.end()); i != e;
          ++i)
      {
        bstr << i->first;
        i->second.write(bstr);
      }

      bstr << (uint32_t)creative_weight_strategy->type()
           << *creative_weight_strategy;      
    }
    
    void
    Selector::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      uint32_t version = 0;
      bstr >> version >> update_number >> group_cap_timeout
           >> group_cap_max_count >> counter_cap_timeout
           >> counter_cap_max_count;
      
      bstr.read_map(conditions);
      bstr.read_map(capped_groups_min_times);

      uint32_t count = 0;
      bstr >> count;

      pages.clear();

      while(count--)
      {
        PageId id = 0;
        bstr >> id;

        pages.insert(std::make_pair(id, Page())).first->second.
          read(bstr, conditions);
      }
      
      uint32_t tp = 0;
      bstr >> tp;

      creative_weight_strategy.reset(
        CreativeWeightStrategy::create(
          (CreativeWeightStrategy::StrategyType)tp));

      bstr >> *creative_weight_strategy;
    }
    
    void
    Selector::add_page(PageIdValue page_id,
                       SlotIdValue slot_first,
                       SlotIdValue slot_last)
      throw(El::Exception)
    {  
      Ad::Page& page =
        pages.insert(std::make_pair(page_id, Ad::Page())).first->second;

      for(size_t i = slot_first; i <= (size_t)slot_last; ++i)
      {
        page.slots[i] = Ad::Slot();
      }
    }
      
    void
    Selector::add_creative(PageIdValue page,
                           SlotIdValue slot,
                           const Creative& creative) throw(El::Exception)
    {
      pages[page].slots[slot].creatives.push_back(creative);
    }
    
    void
    Selector::add_counter(PageIdValue page, const Counter& counter)
      throw(El::Exception)
    {
      pages[page].counters.push_back(counter);
    }
    
    void
    Selector::finalize() throw(El::Exception)
    {
      capped_groups_min_times.clear();
      
      for(PageMap::iterator i(pages.begin()); i != pages.end(); )
      {
        Page& page = i->second;
        
        page.finalize(*this);

        if(page.slots.empty() && page.counters.empty())
        {
          PageMap::iterator cur = i++;
          pages.erase(cur);
        }
        else
        {
          ++i;
        }
      }
    }

    void
    Selector::cleanup_caps(GroupCaps& caps) const throw(El::Exception)
    {
      GroupCapMap& cp = caps.caps;
      CapMinTimeMap::const_iterator je(capped_groups_min_times.end());
      
      for(GroupCapMap::iterator i(cp.begin()), e(cp.end()); i != e; ++i)
      {
        CapMinTimeMap::const_iterator j =
          capped_groups_min_times.find(i->first);

        if(j != je && i->second.time < j->second)
        {
          cp.erase(i);
        }
      }
    }
    
    void
    Selector::select(SelectionContext& context, SelectionResult& result)
      throw(Exception, El::Exception)
    {
      cleanup_caps(context.ad_caps);
      cleanup_caps(context.counter_caps);
      
      PageMap::const_iterator i = pages.find(context.page);

      if(i == pages.end())
      {
        result.ad_caps.swap(context.ad_caps);
        result.counter_caps.swap(context.counter_caps);
        return;
      }

      const Page& page = i->second;

      if(!select_ads(page, context, result))
      {
        result.ad_caps.swap(context.ad_caps);
      }
      
      if(!select_counters(page, context, result))
      {
        result.counter_caps.swap(context.counter_caps);
      }
    }
    
    bool
    Selector::select_ads(const Page& page,
                         const SelectionContext& context,
                         SelectionResult& result)
      throw(Exception, El::Exception)
    {
      if(!page.max_ad_num)
      {
        return false;
      }
      
      const SlotMap& slots = page.slots;
      
      SelectedSlotArray sel_slots;
      sel_slots.reserve(slots.size());

      for(SlotIdArray::const_iterator i(context.slots.begin()),
            e(context.slots.end()); i != e; ++i)
      {
        SlotMap::const_iterator j = slots.find(*i);

        if(j != slots.end())
        {
          const Slot& slot = j->second;
          
          CreativeWeightArray creatives;
          creatives.reserve(slot.creatives.size());
          
          for(CreativeArray::const_iterator i(slot.creatives.begin()),
                e(slot.creatives.end()); i != e; ++i)
          {
            const Creative& creative = *i;
            
            if(creative.weight >= 0.0001 && creative.match(context))
            {
              creatives.push_back(CreativeWeight(&creative, creative.weight));
            }
          }

          creative_weight_strategy->set_weights(creatives);
            
          if(!creatives.empty())
          {
            sel_slots.push_back(SelectedSlot(j->first));
            sel_slots.rbegin()->creatives.swap(creatives);
          }
        }
      }

      if(sel_slots.empty())
      {
        return false;
      }
      
      SelTravContext stc;
        
      for(SelectedSlotArray::const_iterator i(sel_slots.begin()),
          e(sel_slots.end()); i != e; ++i)
      {
        select_best_placement(i,
                              e,
                              page.adv_restrictions,
                              page.max_ad_num,
                              stc);
      }

      result.ads.resize(stc.best.size());

      uint64_t expiration_time =
        context.ad_caps.current_time - group_cap_timeout;

      uint64_t min_time = UINT64_MAX;
      GroupCapMap& res_caps = result.ad_caps.caps;
      
      for(GroupCapMap::const_iterator i(context.ad_caps.caps.begin()),
            e(context.ad_caps.caps.end()); i != e; ++i)
      {
        GroupId id = i->first;
        const GroupCap& cap = i->second;
        
        if(cap.time > expiration_time ||
           capped_groups_min_times.find(id) != capped_groups_min_times.end())
        {
          res_caps.insert(std::make_pair(id, cap));

          if(min_time > cap.time)
          {
            min_time = cap.time;
          }
        }
      }

      result.ad_caps.current_time = context.ad_caps.current_time;
        
      for(SelectionArray::reverse_iterator i(result.ads.rbegin());
          !stc.best.empty(); stc.best.pop())
      {
        const SelectedPlacement& sp = stc.best.top();
        const Creative& cr = *sp.creative;
        Selection& sel = *i++;

        sel.id = cr.id;
        sel.slot = sp.id;
        sel.width = cr.width;
        sel.height = cr.height;
        sel.text = cr.text;
        sel.inject = cr.inject;

        if(cr.group_cap)
        {
          GroupCap& cap =
            res_caps.insert(std::make_pair(cr.group,GroupCap())).first->second;

          cap.time = context.ad_caps.current_time;
          ++cap.count;
        }
      }

      if(result.ad_caps.caps.size() > group_cap_max_count)
      {
        for(GroupCapMap::iterator i(result.ad_caps.caps.begin()),
              e(result.ad_caps.caps.end()); i != e; ++i)
        {
          if(i->second.time == min_time)
          {
            result.ad_caps.caps.erase(i);
          }
        }
      }

      return true;
    }

    bool
    Selector::select_counters(const Page& page,
                              const SelectionContext& context,
                              SelectionResult& result)
      throw(Exception, El::Exception)
    {      
      const CounterArray& counters = page.counters;

      GroupCapMap& res_caps = result.counter_caps.caps;

      SelectedCounterArray& res_counters = result.counters;
      res_counters.reserve(counters.size());
      
      for(CounterArray::const_iterator i(counters.begin()),
            e(counters.end()); i != e; ++i)
      {
        const Counter& counter = *i;
            
        if(counter.match(context))
        {
          res_counters.push_back(
            SelectedCounter(counter.id, counter.text.c_str()));

          if(counter.group_cap)
          {
            res_caps.insert(
              std::make_pair(counter.group,
                             GroupCap(1, context.counter_caps.current_time)));
          }
        }  
      }

      if(res_counters.empty())
      {
        return false;
      }

      uint64_t expiration_time =
        context.counter_caps.current_time - counter_cap_timeout;

      uint64_t min_time = UINT64_MAX;
      
      for(GroupCapMap::const_iterator
            i(context.counter_caps.caps.begin()),
            e(context.counter_caps.caps.end()); i != e; ++i)
      {
        GroupId id = i->first;
        const GroupCap& cap = i->second;
        
        if(cap.time > expiration_time ||
           capped_groups_min_times.find(id) != capped_groups_min_times.end())
        {
          GroupCapMap::iterator ci = res_caps.find(id);

          if(ci == res_caps.end())
          {
            res_caps.insert(std::make_pair(id, cap));

            if(min_time > cap.time)
            {
              min_time = cap.time;
            }
          }
          else
          {
            ci->second.count += cap.count;
          }
        }
      }

      result.counter_caps.current_time = context.counter_caps.current_time;

      if(result.counter_caps.caps.size() > counter_cap_max_count)
      {
        for(GroupCapMap::iterator
              i(result.counter_caps.caps.begin()),
              e(result.counter_caps.caps.end()); i != e; ++i)
        {
          if(i->second.time == min_time)
          {
            result.counter_caps.caps.erase(i);
          }
        }
      }

      return true;
    }

    void
    Selector::select_best_placement(SelectedSlotArray::const_iterator slot_b,
                                    SelectedSlotArray::const_iterator slot_e,
                                    const AdvRestrictionMap& adv_restrictions,
                                    uint32_t level,
                                    SelTravContext& context) const
      throw(El::Exception)
    {
      // For optimization sort ads in slot in ecpm descending order and maybe
      // bailout earlier. Also calculate max_ecpm for each slot and sort
      // slots in value descending order and then bailout earlier.

      // check here if context.current_ecpm + sum of slot_max_ecmp for
      // this slot and below < context.best_ecpm then return


      AdvCountMap& adv_counter = context.adv_counter;
      GroupIdSet& group_capped = context.group_capped;
      double& current_weight = context.current_weight;
      double& best_weight = context.best_weight;
      SelectedPlacementStack& current = context.current;
        
      for(CreativeWeightArray::const_iterator i(slot_b->creatives.begin()),
            e(slot_b->creatives.end()); i != e; ++i)
      {
        const CreativeWeight& cw = *i;
        const Creative* c = cw.creative;
/*
        if(c->weight == 0)
        {
          continue;
        }
*/

        if(c->group_cap && group_capped.find(c->group) != group_capped.end())
        {
          continue;
        }
        
        AdvertiserId advertiser = c->advertiser;
        
        AdvRestrictionMap::const_iterator ai =
          adv_restrictions.find(advertiser);

        uint32_t* counter = 0;

        if(ai != adv_restrictions.end())
        {
          const AdvRestriction& ar = ai->second;          
          
          if(ar.max_ad_num == 0)
          {
            continue;
          }
          
          AdvCountMap::iterator ac = adv_counter.find(advertiser);

          if(ac != adv_counter.end())
          {
            if(ac->second == ar.max_ad_num)
            {
              continue;
            }
          }
          else
          {
            ac = adv_counter.insert(std::make_pair(advertiser, 0)).first;
          }

          counter = &ac->second;
          
          AdvMaxAdNumArray::const_iterator i(ar.adv_max_ad_nums.begin());
          AdvMaxAdNumArray::const_iterator e(ar.adv_max_ad_nums.end());
          
          for(; i != e; ++i)
          {
            const AdvMaxAdNum& am = *i;
            AdvCountMap::iterator ac = adv_counter.find(am.advertiser);

            if(ac != adv_counter.end() &&
               ac->second + *counter >= am.max_ad_num)
            {
              break;
            }
          }

          if(i != e)
          {
            continue;
          }

          ++(*counter);
        }

        if(c->group_cap)
        {
          group_capped.insert(c->group);
        }
        
        double weight = current_weight;
        current_weight += cw.weight;
        current.push(SelectedPlacement(slot_b->id, c));

        if(best_weight < current_weight ||
           (best_weight - current_weight < 0.01 &&
            context.best.size() > current.size()))
        {
          best_weight = current_weight;
          context.best = current;
        }

        if(level > 1)
        {
          for(SelectedSlotArray::const_iterator i(slot_b + 1); i != slot_e;
              ++i)
          {
            select_best_placement(i,
                                  slot_e,
                                  adv_restrictions,
                                  level - 1,
                                  context);
          }
        }

        if(counter)
        {
          --(*counter);
        }
        
        current_weight = weight;
        current.pop();

        if(c->group_cap)
        {
          group_capped.erase(c->group);
        }
      }
    }
    
    void
    Selector::dump(std::ostream& ostr, const char* ident) const
      throw(El::Exception)
    {
      ostr << ident << pages.size() << " pages:";

      for(PageMap::const_iterator i(pages.begin()), e(pages.end());
          i != e; ++i)
      {
        ostr << std::endl << ident << "  pageid " << i->first << ":";
        
        i->second.dump(ostr, (std::string(ident) + "    ").c_str());
      }
    }

    //
    // Counter struct
    //

    void
    Counter::finalize(Selector& selector) throw(El::Exception)
    {
      group_cap = 0;
      
      for(ConditionPtrArray::const_iterator i(conditions.begin()),
            e(conditions.end()); i != e; ++i)
      {
        const Condition& cond = **i;
        
        if(cond.group_freq_cap || cond.group_count_cap)
        {
          group_cap = 1;
          
          selector.capped_groups_min_times.insert(
            std::make_pair(group, group_cap_min_time));
          
          break;
        }
      }
    }
    
    void
    Counter::dump(std::ostream& ostr, const char* ident) const
      throw(El::Exception)
    {
      ostr << std::endl << ident << "id " << id;      
    }

    //
    // Creative struct
    //

    void
    Creative::finalize(Selector& selector) throw(El::Exception)
    {
      group_cap = 0;
      
      for(ConditionPtrArray::const_iterator i(conditions.begin()),
            e(conditions.end()); i != e; ++i)
      {
        const Condition& cond = **i;
        
        if(cond.group_freq_cap || cond.group_count_cap)
        {
          group_cap = 1;
          
          selector.capped_groups_min_times.insert(
            std::make_pair(group, group_cap_min_time));
          
          break;
        }
      }
    }
    
    void
    Creative::dump(std::ostream& ostr, const char* ident) const
      throw(El::Exception)
    {
      ostr << std::endl << ident << "id " << id << " size " << width << "x"
           << height;
    }

    //
    // AdvRestriction struct
    //

    void
    Slot::finalize(Selector& selector) throw(El::Exception)
    {
      for(CreativeArray::iterator i(creatives.begin()), e(creatives.end());
          i != e; ++i)
      {
        i->finalize(selector);
      }
    }
    
    void
    Slot::dump(std::ostream& ostr, const char* ident) const
      throw(El::Exception)
    {
      ostr << std::endl << ident << "creatives:";

      for(CreativeArray::const_iterator i(creatives.begin()),
            e(creatives.end()); i != e; ++i)
      {
        i->dump(ostr, (std::string(ident) + "    ").c_str());
      }
    }
    
    //
    // AdvRestriction struct
    //
    void
    AdvRestriction::dump(std::ostream& ostr, const char* ident)
      const throw(El::Exception)
    {
      ostr << std::endl << ident << max_ad_num << " max_ad_num";      
    }
    
    //
    // Page struct
    //
    void
    Page::finalize(Selector& selector) throw(El::Exception)
    {
      for(SlotMap::iterator i(slots.begin()); i != slots.end(); )
      {
        Ad::Slot& slot = i->second;

        slot.finalize(selector);
        
        if(slot.creatives.empty())
        {
          SlotMap::iterator cur = i++;
          slots.erase(cur);
        }
        else
        {
          ++i;
        }
      }

      for(CounterArray::iterator i(counters.begin()), e(counters.end());
          i != e; ++i)
      {
        i->finalize(selector);
      }
    }

    void
    Page::dump(std::ostream& ostr, const char* ident) const
      throw(El::Exception)
    {
      ostr << std::endl << ident << max_ad_num << " max_ad_num\n" << ident
           << adv_restrictions.size() << " adv_restrictions:";

      for(AdvRestrictionMap::const_iterator i(adv_restrictions.begin()),
            e(adv_restrictions.end()); i != e; ++i)
      {
        ostr << std::endl << ident << "  advid " << i->first << ":";
        i->second.dump(ostr, (std::string(ident) + "    ").c_str());
      }

      ostr << std::endl << ident << "slots:";

      for(SlotMap::const_iterator i(slots.begin()),
            e(slots.end()); i != e; ++i)
      {
        ostr << std::endl << ident << "  slotid " << i->first << ":";
        i->second.dump(ostr, (std::string(ident) + "    ").c_str());
      }
      
      ostr << std::endl << ident << "counters:";

      for(CounterArray::const_iterator i(counters.begin()), e(counters.end());
          i != e; ++i)
      {
        i->dump(ostr, (std::string(ident) + "    ").c_str());        
      }
    }

    void
    Page::add_max_ad_num(AdvertiserId advertiser1,
                         AdvertiserId advertiser2,
                         uint32_t max_ad_num)
      throw(El::Exception)
    {
      add_max_ad_num_asym(advertiser1, advertiser2, max_ad_num);
      add_max_ad_num_asym(advertiser2, advertiser1, max_ad_num);
    }
    
    void
    Page::add_max_ad_num_asym(AdvertiserId advertiser1,
                              AdvertiserId advertiser2,
                              uint32_t max_ad_num)
      throw(El::Exception)
    {
      AdvRestrictionMap::iterator i = adv_restrictions.find(advertiser1);
    
      if(i == adv_restrictions.end())
      {
        i = adv_restrictions.insert(
          std::make_pair(advertiser1, Ad::AdvRestriction(UINT32_MAX))).first;
      }

      i->second.adv_max_ad_nums.push_back(
        AdvMaxAdNum(advertiser2, max_ad_num));
    }

    void
    Page::add_max_ad_num(AdvertiserId advertiser, uint32_t max_ad_num)
      throw(El::Exception)
    {
      AdvRestrictionMap::iterator i = adv_restrictions.find(advertiser);

      if(i == adv_restrictions.end())
      {
        adv_restrictions.insert(
          std::make_pair(advertiser, Ad::AdvRestriction(max_ad_num)));
      }
      else
      {
        Ad::AdvRestriction& ar = i->second;
        ar.max_ad_num = std::min(ar.max_ad_num, max_ad_num);
      }
    }    
    
    //
    // SelectionContext struct
    //
    void
    SelectionContext::add_category(const char* category,
                                   CategorySet& categories)
      throw(El::Exception)
    {
      std::string cat;
      El::String::Manip::to_lower(category, cat);
      category = cat.c_str();
      
      for(const char* end = strchr(category, '/'); end != 0;
          end = strchr(end + 1, '/'))
      {
        categories.insert(std::string(category, end - category + 1));
      }
    }

    void
    SelectionContext::add_url(const char* url, StringSet& urls)
      throw(El::Exception)
    {
      std::string u;

      try
      {
        normalize_url(url, u);
      }
      catch(...)
      {
        return;
      }

      url = u.c_str();
      urls.insert(url);
      
//      std::cerr << source << std::endl;

      for(const char* end = strchr(url, '.'); end != 0;
          end = strchr(end + 1, '.'))
      {
        std::string u = std::string(url, end - url) + "/";
//        std::cerr << src << std::endl;
  
        urls.insert(u);
      }

      for(const char* end = strchr(url, '/'); end != 0;
          end = strchr(end + 1, '/'))
      {
        {
          std::string u(url, end - url);
//          std::cerr << src << std::endl;
  
          urls.insert(u);
        }
        
        {
          std::string u(url, end - url + 1);
//          std::cerr << src << std::endl;

          urls.insert(u);
        }
        
      }

      const char* sep = strchr(url, '?');

      if(sep)
      {
        std::string u(url, sep - url);
//        std::cerr << src << std::endl;
  
        urls.insert(u);
      }      
      
//      std::cerr << "---------------" << std::endl;
    }

    //
    // Selection struct
    //
    void
    Selection::dump(std::ostream& ostr) const throw(El::Exception)
    {
      ostr << "(" << slot << "," << width << "x" << height << ","
           << text << ")";
    }

    //
    // SelectedCounter struct
    //
    void
    SelectedCounter::dump(std::ostream& ostr) const throw(El::Exception)
    {
      ostr << "(" << text << ")";
    }

    //
    // SelectionResult struct
    //
    void
    SelectionResult::dump(std::ostream& ostr) const throw(El::Exception)
    {
      ostr << "ads:";
      
      for(SelectionArray::const_iterator i(ads.begin()), e(ads.end()); i != e;
          ++i)
      {
        ostr << " ";
        i->dump(ostr);
      }

      ostr << ", counters:";
      
      for(SelectedCounterArray::const_iterator i(counters.begin()),
            e(counters.end()); i != e; ++i)
      {
        ostr << " ";
        i->dump(ostr);
      }
    }

    //
    // Utility functions
    //
    void
    normalize_url(const char* url, std::string& normalized)
      throw(El::Exception)
    {
      std::string lowered;
      El::String::Manip::to_lower(url, lowered);
      
      El::Net::HTTP::URL_var u = new El::Net::HTTP::URL(lowered.c_str());

      const char* domain = u->host();
      const char* prev = domain + strlen(domain);

      for(const char* ptr = prev - 1; ptr != domain; ptr--)
      {
        if(*ptr == '.')
        {
          std::string part(ptr + 1, prev - ptr - 1);
          
          if(!normalized.empty())
          {
            normalized += ".";
          }
          
          normalized += part;
          prev = ptr;
        }
      }      

      if(domain != prev)
      {
        std::string part(domain, prev - domain);

        if(!normalized.empty())
        {
          normalized += ".";
        }
        
        normalized += part;
      }

      normalized += u->path();

      if(*u->params())
      {
        normalized += std::string("?") + u->params();
      }
      
    }
  }
}
