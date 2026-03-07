#!/usr/bin/env python3
"""
Progress tracker for ScratchRobin implementation.
Reads IMPLEMENTATION_TRACKER.csv and generates status reports.
"""

import csv
import sys
from pathlib import Path
from collections import defaultdict


def read_tracker():
    """Read the tracker CSV file."""
    tracker_path = Path(__file__).parent.parent / "docs" / "IMPLEMENTATION_TRACKER.csv"
    
    if not tracker_path.exists():
        print(f"Error: Tracker file not found at {tracker_path}")
        sys.exit(1)
    
    features = []
    with open(tracker_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            features.append(row)
    
    return features


def generate_summary(features):
    """Generate overall summary statistics."""
    total = len(features)
    
    by_status = defaultdict(int)
    by_phase = defaultdict(int)
    by_phase_complete = defaultdict(int)
    by_priority = defaultdict(int)
    
    total_hours = 0
    completed_hours = 0
    
    for f in features:
        status = f.get('Status', 'Not Started')
        phase = f.get('Phase', 'Unknown')
        priority = f.get('Priority', 'Unknown')
        hours = int(f.get('Estimated Hours', '0') or '0')
        
        by_status[status] += 1
        by_phase[phase] += 1
        by_priority[priority] += 1
        total_hours += hours
        
        if status == 'Completed':
            by_phase_complete[phase] += 1
            completed_hours += hours
    
    completed = by_status.get('Completed', 0)
    in_progress = by_status.get('In Progress', 0)
    not_started = by_status.get('Not Started', 0)
    
    return {
        'total': total,
        'completed': completed,
        'in_progress': in_progress,
        'not_started': not_started,
        'by_phase': dict(by_phase),
        'by_phase_complete': dict(by_phase_complete),
        'by_status': dict(by_status),
        'by_priority': dict(by_priority),
        'total_hours': total_hours,
        'completed_hours': completed_hours,
    }


def print_summary(summary):
    """Print formatted summary."""
    print("=" * 70)
    print("SCRATCHROBIN IMPLEMENTATION STATUS")
    print("=" * 70)
    print()
    
    # Overall progress
    total = summary['total']
    completed = summary['completed']
    in_progress = summary['in_progress']
    not_started = summary['not_started']
    
    percent_complete = (completed / total * 100) if total > 0 else 0
    percent_in_progress = (in_progress / total * 100) if total > 0 else 0
    
    print(f"Overall Progress: {percent_complete:.1f}%")
    print(f"  Completed:   {completed:3d} / {total} ({percent_complete:.1f}%)")
    print(f"  In Progress: {in_progress:3d} / {total} ({percent_in_progress:.1f}%)")
    print(f"  Not Started: {not_started:3d} / {total}")
    print()
    
    # By phase
    print("-" * 70)
    print("Progress by Phase")
    print("-" * 70)
    
    phase_names = {
        '1': 'Phase 1: Core UI',
        '2': 'Phase 2: Data Management',
        '3': 'Phase 3: SQL Editor',
        '4': 'Phase 4: Advanced Features',
        '5': 'Phase 5: Polish',
    }
    
    for phase_num in ['1', '2', '3', '4', '5']:
        phase_total = summary['by_phase'].get(phase_num, 0)
        phase_complete = summary['by_phase_complete'].get(phase_num, 0)
        percent = (phase_complete / phase_total * 100) if phase_total > 0 else 0
        
        name = phase_names.get(phase_num, f'Phase {phase_num}')
        print(f"  {name:30s} {percent:5.1f}% ({phase_complete}/{phase_total})")
    
    print()
    
    # By priority
    print("-" * 70)
    print("Features by Priority")
    print("-" * 70)
    
    for priority in ['Critical', 'High', 'Medium', 'Low']:
        count = summary['by_priority'].get(priority, 0)
        print(f"  {priority:10s}: {count:3d} features")
    
    print()
    
    # Effort
    print("-" * 70)
    print("Effort Summary")
    print("-" * 70)
    print(f"  Total Estimated Hours:   {summary['total_hours']:4d}")
    print(f"  Completed Hours:         {summary['completed_hours']:4d}")
    print(f"  Remaining Hours:         {summary['total_hours'] - summary['completed_hours']:4d}")
    
    if summary['total_hours'] > 0:
        hours_percent = summary['completed_hours'] / summary['total_hours'] * 100
        print(f"  Hours Complete:          {hours_percent:5.1f}%")
    
    print()
    print("=" * 70)


def main():
    """Main entry point."""
    features = read_tracker()
    summary = generate_summary(features)
    print_summary(summary)


if __name__ == '__main__':
    main()
